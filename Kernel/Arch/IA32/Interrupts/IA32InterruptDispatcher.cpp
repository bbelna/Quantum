/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptDispatcher.cpp
 * @brief Implements @ref @QKrnlIA32::Interrupts::IA32InterruptDispatcher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/Process.hpp>
#include <Concurrency/ProcessManager.hpp>
#include <Concurrency/ThreadManager.hpp>
#include <KernelContext.hpp>
#include <KernelLog.hpp>
#include <Memory/IMemoryAllocator.hpp>
#include <Memory/IMemoryMapper.hpp>
#include <Memory/Stack.hpp>

#include "IA32InterruptConstants.hpp"
#include "IA32IDT.hpp"
#include "IA32InterruptDispatcher.hpp"
#include "IA32SystemCallHandler.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  IA32InterruptDispatcher::IA32InterruptDispatcher(
    IA32IDT* idt,
    KernelContext* kernelContext
  ) {
    _idt = idt;
    _kernelContext = kernelContext;
    _systemCallHandler = new IA32SystemCallHandler(kernelContext);

    _idt->SetDispatcher(this);
  }

  IA32InterruptContext* IA32InterruptDispatcher::Dispatch(
    IA32InterruptContext& context
  ) {
    switch (context.Vector) {
      case IRQ_VECTOR_DIVIDE_BY_ZERO: {
        return _divideByZero(context);
      }

      case IRQ_VECTOR_INVALID_OPCODE: {
        return _invalidOpCode(context);
      }

      case IRQ_VECTOR_GENERAL_PROTECTION_FAULT: {
        return _generalProtectionFault(context);
      }

      case IRQ_VECTOR_PAGE_FAULT: {
        return _pageFault(context);
      }

      case IRQ_VECTOR_SYSTEM_CALL: {
        return _systemCallHandler->Handle(context);
      }

      default: {
        return const_cast<IA32InterruptContext*>(&context);
      }
    }
  }

  IA32InterruptContext* IA32InterruptDispatcher::_divideByZero(
    IA32InterruptContext& context
  ) {
    if (IA32InterruptContext::IsUserModeContext(context)) {
      Process* process = _kernelContext->ProcessesManager->GetCurrent();

      KLOG_ERROR(
        "Divide-by-zero in PID %u (%s) at EIP %p",
        process->ID,
        process->Name,
        context.EIP
      );

      _kernelContext->ProcessesManager->Terminate(
        process,
        -8
      );

      return static_cast<IA32InterruptContext*>(
        _kernelContext->ThreadManager->Terminate(
          &context,
          -8
        )
      );
    }

    MAYDAY("Kernel divide-by-zero fault");
  }

  IA32InterruptContext* IA32InterruptDispatcher::_invalidOpCode(
    IA32InterruptContext& context
  ) {
    if (IA32InterruptContext::IsUserModeContext(context)) {
      Process* process = _kernelContext->ProcessesManager->GetCurrent();

      KLOG_ERROR(
        "Invalid opcode in PID %u (%s) at EIP %p",
        process->ID,
        process->Name,
        context.EIP
      );

      _kernelContext->ProcessesManager->Terminate(
        process,
        -4
      );

      return static_cast<IA32InterruptContext*>(
        _kernelContext->ThreadManager->Terminate(
          &context,
          -4
        )
      );
    }

    MAYDAY("Kernel invalid opcode fault");
  }

  IA32InterruptContext* IA32InterruptDispatcher::_generalProtectionFault(
    IA32InterruptContext& context
  ) {
    if (IA32InterruptContext::IsUserModeContext(context)) {
      Process* process = _kernelContext->ProcessesManager->GetCurrent();

      KLOG_ERROR(
        "GPF in PID %u (%s): EIP=%p, ErrorCode=%p, CS=%p",
        process->ID,
        process->Name,
        context.EIP,
        context.ErrorCode,
        context.CS
      );

      _kernelContext->ProcessesManager->Terminate(
        process,
        -11
      );

      return static_cast<IA32InterruptContext*>(
        _kernelContext->ThreadManager->Terminate(
          &context,
          -11
        )
      );
    }

    KLOG_ERROR(
      "Error: General Protection Fault (GPF)\n  EIP=%p, ErrorCode=%p, CS=%p",
      context.EIP,
      context.ErrorCode,
      context.CS
    );
    KLOG_ERROR(
      "  EAX=%p EBX=%p ECX=%p EDX=%p",
      context.EAX,
      context.EBX,
      context.ECX,
      context.EDX
    );
    KLOG_ERROR(
      "  ESI=%p EDI=%p EBP=%p ESP=%p",
      context.ESI,
      context.EDI,
      context.EBP,
      context.ESP
    );
    KLOG_ERROR(
      "  DS=%p ES=%p FS=%p GS=%p EFLAGS=%p",
      context.DS,
      context.ES,
      context.FS,
      context.GS,
      context.EFLAGS
    );
    KLOG_ERROR(
      "  InterruptContext at %p (vector %u)",
      reinterpret_cast<UInt32>(&context),
      context.Vector
    );

    UInt32 msrLow, msrHigh;

    asm volatile("rdmsr" : "=a"(msrLow), "=d"(msrHigh) : "c"(0x175));

    UInt32 sysenterESP = msrLow;
    UInt32 currentESP;

    asm volatile("mov %%esp, %0" : "=r"(currentESP));

    KLOG_ERROR(
      "  SYSENTER_ESP=%p, current ESP=%p",
      sysenterESP,
      currentESP
    );

    Thread* thread
      = _kernelContext->ThreadManager
      ? _kernelContext->ThreadManager->GetCurrent()
      : nullptr;

    if (
      thread &&
      thread->KernelStack
    ) {
      UInt32 stackBase = thread->KernelStack->Base;
      UInt32 stackTop
        = stackBase
        + thread->KernelStack->SizeInBytes;
      bool contextInStack
        = reinterpret_cast<UInt32>(&context) >= stackBase
       && reinterpret_cast<UInt32>(&context) < stackTop;
      bool espInStack 
        = currentESP >= stackBase
       && currentESP < stackTop;
      bool sysenterInStack
        = sysenterESP >= stackBase
       && sysenterESP < stackTop;

      KLOG_ERROR(
        "  Thread %u stack: %p-%p, ctx_in=%u esp_in=%u sysenter_in=%u",
        thread->ID,
        stackBase,
        stackTop,
        contextInStack
          ? 1u
          : 0u,
        espInStack
          ? 1u
          : 0u,
        sysenterInStack
          ? 1u
          : 0u
      );
    }

    MAYDAY("General Protection Fault (GPF)");
  }

  IA32InterruptContext* IA32InterruptDispatcher::_pageFault(
    IA32InterruptContext& context
  ) {
    UInt32 faultAddress;

    // read faulting address from CR2
    asm volatile("mov %%cr2, %0" : "=r"(faultAddress));

    UInt32 errorCode = context.ErrorCode;
    bool isUserMode = (errorCode & 0x4) != 0;
    bool isPresent = (errorCode & 0x1) != 0;

    // kernel-mode page fault: recoverable if it occurred on behalf of a
    // non-kernel process (e.g. during a syscall), fatal otherwise
    if (!isUserMode) {
      Process* faultProcess
        = _kernelContext->ProcessesManager
        ? _kernelContext->ProcessesManager->GetCurrent()
        : nullptr;

      bool recoverable
        = faultProcess
       && faultProcess != _kernelContext->ProcessesManager->GetKernelProcess();

      if (recoverable) {
        KLOG_ERROR(
          "Page fault on behalf of PID %u (%s) at %p "
          "(EIP %p, ErrorCode %p), terminating process",
          faultProcess->ID,
          faultProcess->Name,
          faultAddress,
          context.EIP,
          errorCode
        );

        KLOG_ERROR(
          "  EAX=%p EBX=%p ECX=%p EDX=%p\n"
          "  ESI=%p EDI=%p EBP=%p EFLAGS=%p",
          context.EAX,
          context.EBX,
          context.ECX,
          context.EDX,
          context.ESI,
          context.EDI,
          context.EBP,
          context.EFLAGS
        );

        _logUserFaultContext(
          context,
          faultProcess,
          faultAddress
        );

        _kernelContext->ProcessesManager->Terminate(faultProcess, -11);

        return static_cast<IA32InterruptContext*>(
          _kernelContext->ThreadManager->Terminate(&context, -11)
        );
      }

      KLOG_ERROR(
        "Page fault at %p (EIP %p, ErrorCode %p)\n"
        "  EAX=%p EBX=%p ECX=%p EDX=%p\n"
        "  ESI=%p EDI=%p EBP=%p ESP=%p\n"
        "  DS=%p ES=%p FS=%p GS=%p EFLAGS=%p",
        faultAddress,
        context.EIP,
        errorCode,
        context.EAX,
        context.EBX,
        context.ECX,
        context.EDX,
        context.ESI,
        context.EDI,
        context.EBP,
        context.ESP,
        context.DS,
        context.ES,
        context.FS,
        context.GS,
        context.EFLAGS
      );

      // dump 32 dwords upward from the fault ESP so we can see the frame
      // that was live at crash time (e.g. the ports parameter in ReceiveAny)
      Thread* thread
        = _kernelContext->ThreadManager
        ? _kernelContext->ThreadManager->GetCurrent()
        : nullptr;

      if (
        thread &&
        thread->KernelStack
      ) {
        UInt32 faultESP = context.ESP;
        UInt32 stackBase = thread->KernelStack->Base;
        UInt32 stackTop
          = thread->KernelStack->Base
          + thread->KernelStack->SizeInBytes;

        if (faultESP >= stackBase && faultESP < stackTop) {
          volatile UInt32* faultESPPtr = reinterpret_cast<volatile UInt32*>(
            faultESP
          );
          Size dwords = (stackTop - faultESP) / 4;

          if (dwords > 64) {
            dwords = 64;
          }

          KLOG_ERROR(
            "  Fault-ESP stack dump (%u dwords):",
            dwords
          );

          for (Size i = 0; i < dwords; i += 4) {
            KLOG_ERROR(
              "    [%p]: %p %p %p %p",
              faultESP + i * 4,
              faultESPPtr[i],
              faultESPPtr[i + 1],
              faultESPPtr[i + 2],
              faultESPPtr[i + 3]
            );
          }
        }
      }

      MAYDAY("Page fault");
    }

    Process* process = _kernelContext->ProcessesManager->GetCurrent();

    if (!process) {
      KLOG_ERROR(
        "Page fault at %p with no current process (EIP %p)",
        faultAddress,
        context.EIP
      );

      MAYDAY("Page fault with no current process");
    }

    // look up the faulting address in the process' address space map
    Result<MemoryMapping> findResult
      = process->AddressSpaceMap.FindContainingBlock(faultAddress);

    // no mapping found -> segmentation fault, terminate the process
    if (!findResult.Success) {
      KLOG_ERROR(
        "Segmentation fault in PID %u (%s): unmapped address %p (EIP %p)",
        process->ID,
        process->Name,
        faultAddress,
        context.EIP
      );

      _logUserFaultContext(
        context,
        process,
        faultAddress
      );

      _kernelContext->ProcessesManager->Terminate(
        process,
        -11
      );

      return static_cast<IA32InterruptContext*>(
        _kernelContext->ThreadManager->Terminate(
          &context,
          -11
        )
      );
    }

    MemoryMapping mapping = findResult.Data;

    // guard region -> terminate the process
    if (
      Enum::HasFlag(
        mapping.Flags.Options,
        MemoryMappingOptions::Guard
      ) ||
      mapping.RegionType == MemoryRegionType::Guard
    ) {
      KLOG_ERROR(
        "Guard page violation in PID %u (%s): address %p (EIP %p)",
        process->ID,
        process->Name,
        faultAddress,
        context.EIP
      );

      _logUserFaultContext(
        context,
        process,
        faultAddress
      );

      _kernelContext->ProcessesManager->Terminate(
        process,
        -11
      );

      return static_cast<IA32InterruptContext*>(
        _kernelContext->ThreadManager->Terminate(
          &context,
          -11
        )
      );
    }

    // lazy mapping with block not present -> demand block/page allocation
    if (
      Enum::HasFlag(
        mapping.Flags.Options,
        MemoryMappingOptions::Lazy
      ) &&
      !isPresent
    ) {
      Size blockSize = _kernelContext->MemoryAllocator->GetBlockSize();
      UIntPtr blockBase = AlignDown(
        faultAddress,
        static_cast<UInt32>(blockSize)
      );
      MemoryBlock block = _kernelContext->MemoryAllocator->Allocate(
        MemoryBlockTag::DemandPage
      );

      // allocation failed -> out of memory, terminate the process
      if (block.Base == 0) {
        KLOG_ERROR(
          "Out of memory during demand allocation in PID %u (%s) at %p",
          process->ID,
          process->Name,
          faultAddress
        );

        _kernelContext->ProcessesManager->Terminate(
          process,
          -12
        );

        return static_cast<IA32InterruptContext*>(
          _kernelContext->ThreadManager->Terminate(
            &context,
            -12
          )
        );
      }

      // map without the Lazy flag, the page table entry is eagerly present
      MemoryMappingFlags mapFlags {
        mapping.Flags.Permissions,
        mapping.Flags.Cache,
        MemoryMappingOptions::None
      };

      // map the block at the same process address it was originally requested
      // at, which is also the faulting address rounded down to the block size
      // this allows the process to access the block at the intended address
      // without needing to track a separate process address for the lazy block
      MemoryBlock mapped = _kernelContext->MemoryMapper->Map(
        *process->AddressSpace,
        MemoryBlock {
          blockBase,
          blockSize
        },
        block,
        mapFlags
      );

      // mapping failed (e.g. process address already mapped, or page table
      // entry already)
      if (mapped.SizeInBytes == 0) {
        _kernelContext->MemoryAllocator->Free(block);

        KLOG_ERROR(
          "Failed to map demand page in PID %u (%s) at %p",
          process->ID,
          process->Name,
          faultAddress
        );

        _kernelContext->ProcessesManager->Terminate(
          process,
          -12
        );

        return static_cast<IA32InterruptContext*>(
          _kernelContext->ThreadManager->Terminate(
            &context,
            -12
          )
        );
      }

      // zero the page for security
      Byte::Zero(
        reinterpret_cast<void*>(blockBase),
        blockSize
      );

      // update the memory tracking
      process->HeapBlockCount++;
      process->TotalBlockCount++;

      // retry the faulting instruction
      return &context;
    }

    // all other cases: access violation on a present or non-lazy page
    KLOG_ERROR(
      "Access violation in PID %u (%s): address %p "
      "(EIP %p, ErrorCode %p)",
      process->ID,
      process->Name,
      faultAddress,
      context.EIP,
      errorCode
    );

    _logUserFaultContext(
      context,
      process,
      faultAddress
    );

    _kernelContext->ProcessesManager->Terminate(
      process,
      -11
    );

    return static_cast<IA32InterruptContext*>(
      _kernelContext->ThreadManager->Terminate(
        &context,
        -11
      )
    );
  }

  void IA32InterruptDispatcher::_logUserFaultContext(
    const IA32InterruptContext& context,
    const Process* process,
    UInt32 faultAddress
  ) {
    // register dump
    KLOG_ERROR(
      "  EAX=%p EBX=%p ECX=%p EDX=%p",
      context.EAX,
      context.EBX,
      context.ECX,
      context.EDX
    );

    KLOG_ERROR(
      "  ESI=%p EDI=%p EBP=%p EFLAGS=%p",
      context.ESI,
      context.EDI,
      context.EBP,
      context.EFLAGS
    );

    // for ring-3 faults the CPU pushes ESP and SS after EFLAGS
    bool isUserMode = IA32InterruptContext::IsUserModeContext(context);
    UInt32 userESP = 0;

    if (isUserMode) {
      const UInt32* frame = &context.EFLAGS;

      userESP = frame[1];

      KLOG_ERROR(
        "  User ESP=%p  SS=%p",
        userESP,
        frame[2]
      );
    } else {
      userESP = context.ESP;

      KLOG_ERROR("  ESP=%p", userESP);
    }

    // user stack dump (the process page directory is still active)
    if (userESP > 0) {
      Result<MemoryMapping> stackResult
        = process->AddressSpaceMap.FindContainingBlock(userESP);

      if (!stackResult.Success) {
        KLOG_ERROR("  (user stack not mapped at ESP)");

        return;
      }

      MemoryMapping stackMapping = stackResult.Data;
      UInt32 stackTop
        = stackMapping.ProcessBlock.Base
        + stackMapping.ProcessBlock.SizeInBytes;

      // cap at 16 dwords (64 bytes) of stack
      UInt32 dumpBytes = stackTop - userESP;

      if (dumpBytes > 64) {
        dumpBytes = 64;
      }

      Size dwords = dumpBytes / 4;

      if (dwords == 0) return;

      const volatile UInt32* stackPointer
        = reinterpret_cast<const volatile UInt32*>(userESP);

      KLOG_ERROR(
        "  User stack (%u dwords from ESP):",
        dwords
      );

      for (Size i = 0; i < dwords; i += 4) {
        Size remaining = dwords - i;

        if (remaining >= 4) {
          KLOG_ERROR(
            "    [%p]: %p %p %p %p",
            userESP + i * 4,
            stackPointer[i],
            stackPointer[i + 1],
            stackPointer[i + 2],
            stackPointer[i + 3]
          );
        } else if (remaining == 3) {
          KLOG_ERROR(
            "    [%p]: %p %p %p",
            userESP + i * 4,
            stackPointer[i],
            stackPointer[i + 1],
            stackPointer[i + 2]
          );
        } else if (remaining == 2) {
          KLOG_ERROR(
            "    [%p]: %p %p",
            userESP + i * 4,
            stackPointer[i],
            stackPointer[i + 1]
          );
        } else {
          KLOG_ERROR(
            "    [%p]: %p",
            userESP + i * 4,
            stackPointer[i]
          );
        }
      }
    }
  }
}
