/**
 * @file Kernel/Arch/IA32/Concurrency/IA32ThreadContextManager.cpp
 * @brief Implements @ref @QKrnlIA32::Concurrency::ThreadContextManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/Drivers/CPU/IA32CPUDriver.hpp>
#include <Arch/IA32/Interrupts/IA32InterruptContext.hpp>
#include <Arch/IA32/Interrupts/IA32InterruptTypes.hpp>
#include <Concurrency/Process.hpp>
#include <Concurrency/Thread.hpp>
#include <KernelLog.hpp>
#include <Memory/Stack.hpp>

#include "IA32ThreadContextManager.hpp"

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  IA32ThreadContextManager::IA32ThreadContextManager(
    IA32CPUDriver* cpu,
    IA32TaskStateSegmentManager* tssManager,
    IAddressSpace* kernelAddressSpace,
    IMaydayHandler* maydayHandler
  ) :
    _cpu(cpu),
    _tssManager(tssManager),
    _kernelAddressSpace(kernelAddressSpace),
    _maydayHandler(maydayHandler) {}

  bool IA32ThreadContextManager::InitializeContext(
    Thread* thread,
    UIntPtr entryPoint,
    UIntPtr argument,
    UIntPtr stackTop,
    bool isKernelThread
  ) {
    if (!thread) {
      KLOG_ERROR("Cannot initialize thread context for null thread");

      return false;
    }

    // build an interrupt context on the stack that iret can return to
    // stack layout (growing down from stackTop):
    //   [argument]     <- for entry point function
    //   [return addr]  <- dummy, thread should never return here
    //   [EFLAGS]
    //   [CS]
    //   [EIP]          <- entry point
    //   [Error Code]   <- dummy
    //   [Vector]       <- dummy
    //   [EAX-EDI]      <- pusha layout

    UInt32 stackPtr = stackTop;

    // push argument for the entry function
    stackPtr -= sizeof(UInt32);
    *reinterpret_cast<UInt32*>(stackPtr) = argument;

    // push dummy return address
    stackPtr -= sizeof(UInt32);
    *reinterpret_cast<UInt32*>(stackPtr) = 0;

    // now build the IInterruptContext structure
    // we need space for the full IInterruptContext
    stackPtr -= sizeof(IA32InterruptContext);

    IA32InterruptContext* context = reinterpret_cast<IA32InterruptContext*>(stackPtr);

    // segment registers (kernel data segment for initial context)
    context->GS = KernelDataSegment;
    context->FS = KernelDataSegment;
    context->ES = KernelDataSegment;
    context->DS = KernelDataSegment;

    // initialize general purpose registers to 0
    context->EDI = 0;
    context->ESI = 0;
    context->EBP = 0;
    context->ESP = stackPtr + sizeof(IA32InterruptContext);
    context->EBX = 0;
    context->EDX = 0;
    context->ECX = 0;
    context->EAX = 0;

    // interrupt metadata
    context->Vector = 0;
    context->ErrorCode = 0;

    // instruction pointer - where execution begins
    context->EIP = entryPoint;

    // code segment
    context->CS = isKernelThread ? KernelCodeSegment : UserCodeSegment;

    // EFLAGS with interrupts enabled
    context->EFLAGS = DefaultEFLAGS;

    // store the context pointer in the thread
    thread->Context = context;

    return true;
  }

  bool IA32ThreadContextManager::InitializeEmptyContext(
    Thread* thread,
    UIntPtr stackTop
  ) {
    if (!thread) {
      KLOG_ERROR("Cannot initialize empty context for null thread");

      return false;
    }

    UInt32 stackPtr = stackTop - sizeof(IA32InterruptContext);
    IA32InterruptContext* context = reinterpret_cast<IA32InterruptContext*>(stackPtr);

    // zero out the context
    for (Size i = 0; i < sizeof(IA32InterruptContext) / sizeof(UInt32); i++) {
      reinterpret_cast<UInt32*>(context)[i] = 0;
    }

    // set kernel data segments (user mode threads start in kernel trampoline)
    context->DS = KernelDataSegment;
    context->ES = KernelDataSegment;
    context->FS = KernelDataSegment;
    context->GS = KernelDataSegment;

    thread->Context = context;

    return true;
  }

  IInterruptContext* IA32ThreadContextManager::SwitchContext(
    IInterruptContext* fromContext,
    Thread* toThread
  ) {
    fromContext = reinterpret_cast<IA32InterruptContext*>(fromContext);

    if (!toThread || !toThread->Context) {
      KLOG_ERROR(
        "Cannot switch context to invalid target thread or context"
      );

      return fromContext;
    }

    // switch to the thread's process address space if available
    auto* targetAddressSpace = _kernelAddressSpace;

    if (toThread->OwnerProcess && toThread->OwnerProcess->AddressSpace) {
      targetAddressSpace = toThread->OwnerProcess->AddressSpace;
    }

    if (targetAddressSpace) {
      UIntPtr kernelPageDirectory = _cpu->VirtualToPhysical(
        reinterpret_cast<UIntPtr>(targetAddressSpace)
      );

      _cpu->LoadPageDirectory(kernelPageDirectory);
    }

    // update TSS ESP0 to point to the new thread's kernel stack top
    // this ensures interrupts in user mode use the correct kernel stack
    if (_tssManager && toThread->KernelStack) {
      UIntPtr kernelStackTop = toThread->KernelStack->Base
                             + toThread->KernelStack->SizeInBytes;

      _tssManager->SetKernelStack(kernelStackTop);
    }

    // validate that the target context is within the thread's kernel stack
    if (toThread->KernelStack) {
      UInt32 ctxAddr = reinterpret_cast<UInt32>(toThread->Context);
      UInt32 stackBase = toThread->KernelStack->Base;
      UInt32 stackTop = stackBase + toThread->KernelStack->SizeInBytes;

      if (ctxAddr < stackBase || ctxAddr >= stackTop) {
        KLOG_ERROR(
          "Context switch: thread %u (%s) context %p outside stack %p-%p",
          toThread->ID,
          toThread->Name,
          ctxAddr,
          stackBase,
          stackTop
        );

        MAYDAY("Context switch target has corrupted context pointer");
      }
    }

    // the interrupt return will use the new thread's context
    return reinterpret_cast<IInterruptContext*>(toThread->Context);
  }

  bool IA32ThreadContextManager::CloneContext(
    const Thread* source,
    Thread* destination
  ) {
    if (
      !source ||
      !destination ||
      !source->Context ||
      !destination->KernelStack
    ) return false;

    // allocate space on destination's stack for the context
    UIntPtr stackTop
      = destination->KernelStack->Base
      + destination->KernelStack->SizeInBytes;

    UInt32 stackPtr = stackTop - sizeof(IA32InterruptContext);
    IA32InterruptContext* newContext
      = reinterpret_cast<IA32InterruptContext*>(stackPtr);

    // copy the context
    const IA32InterruptContext* sourceContext
      = static_cast<const IA32InterruptContext*>(source->Context);

    newContext->GS = sourceContext->GS;
    newContext->FS = sourceContext->FS;
    newContext->ES = sourceContext->ES;
    newContext->DS = sourceContext->DS;
    newContext->EDI = sourceContext->EDI;
    newContext->ESI = sourceContext->ESI;
    newContext->EBP = sourceContext->EBP;
    newContext->ESP = sourceContext->ESP;
    newContext->EBX = sourceContext->EBX;
    newContext->EDX = sourceContext->EDX;
    newContext->ECX = sourceContext->ECX;
    newContext->EAX = sourceContext->EAX;
    newContext->Vector = sourceContext->Vector;
    newContext->ErrorCode = sourceContext->ErrorCode;
    newContext->EIP = sourceContext->EIP;
    newContext->CS = sourceContext->CS;
    newContext->EFLAGS = sourceContext->EFLAGS;

    destination->Context = newContext;

    return true;
  }

  UIntPtr IA32ThreadContextManager::GetInstructionPointer(
    const IInterruptContext* context
  ) const {
    if (!context) return 0;

    return static_cast<const IA32InterruptContext*>(context)->EIP;
  }

  UIntPtr IA32ThreadContextManager::GetStackPointer(
    const IInterruptContext* context
  ) const {
    if (!context) return 0;

    return static_cast<const IA32InterruptContext*>(context)->ESP;
  }

  void IA32ThreadContextManager::SetReturnValue(
    IInterruptContext* context,
    UInt32 value
  ) {
    if (!context) return;

    reinterpret_cast<IA32InterruptContext*>(context)->EAX = value;
  }

  IA32TaskStateSegmentManager* IA32ThreadContextManager::GetTSSManager() const {
    return _tssManager;
  }
}
