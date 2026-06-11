/**
 * @file Kernel/Arch/IA32/Interrupts/InterruptDescriptorTable.cpp
 * @brief Implements @ref @QKrnlIA32::Interrupts::InterruptDescriptorTable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Drivers/Interrupts/IInterruptControllerDriver.hpp>
#include <Interrupts/IInterruptManager.hpp>
#include <KernelLog.hpp>

#include "IA32IDT.hpp"
#include "IA32InterruptDispatcher.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  IA32IDT::IA32IDT(
    IInterruptControllerDriver<IA32InterruptVector>* controller
  ) :
    _controller(controller),
    _exceptionStubs {
      ISR00, ISR01, ISR02, ISR03, ISR04, ISR05, ISR06, ISR07,
      ISR08, ISR09, ISR10, ISR11, ISR12, ISR13, ISR14, ISR15,
      ISR16, ISR17, ISR18, ISR19, ISR20, ISR21, ISR22, ISR23,
      ISR24, ISR25, ISR26, ISR27, ISR28, ISR29, ISR30, ISR31
    },
    _irqStubs {
      IRQ00, IRQ01, IRQ02, IRQ03, IRQ04, IRQ05, IRQ06, IRQ07,
      IRQ08, IRQ09, IRQ10, IRQ11, IRQ12, IRQ13, IRQ14, IRQ15
    }
  {
    _clearEntriesAndHandlers();

    for (
      UInt8 exceptionIndex = 0;
      exceptionIndex < ISR_EXCEPTION_COUNT;
      ++exceptionIndex
    ) {
      _setGate(exceptionIndex, _exceptionStubs[exceptionIndex], 0x8E);
    }

    for (UInt8 irqIndex = 0; irqIndex < IRQ_COUNT; ++irqIndex) {
      _setGate(IRQ_BASE_VECTOR + irqIndex, _irqStubs[irqIndex], 0x8E);
    }

    _setGate(IRQ_VECTOR_YIELD, YIELD49, 0x8E);
    _setGate(IRQ_VECTOR_SYSTEM_CALL, SYSCALL80, 0xEE);

    _descriptor.Limit = sizeof(_entries) - 1;
    _descriptor.Base = reinterpret_cast<UInt32>(&_entries[0]);

    LoadIDT(&_descriptor);

    KLOG_TRACE(
      "Interrupt descriptor table initialized with base %p and limit %u",
      reinterpret_cast<void*>(_descriptor.Base),
      _descriptor.Limit
    );
  }

  void IA32IDT::SetHandler(
    UInt8 vector,
    InterruptHandler handler
  ) {
    _handlerTable[vector] = handler;
  }

  void IA32IDT::SetDispatcher(IA32InterruptDispatcher* dispatcher) {
    _dispatcher = dispatcher;
  }

  IA32InterruptContext* IA32IDT::Dispatch(
    IA32InterruptContext* context
  ) {
    UInt8 vector = static_cast<UInt8>(context->Vector);
    bool isIRQ = vector >= IRQ_BASE_VECTOR
              && vector < (IRQ_BASE_VECTOR + IRQ_COUNT);
    IA32InterruptContext* nextContext = _dispatch(*context);

    if (!nextContext && !isIRQ) {
      KLOG_CRITICAL(
        "Unhandled interrupt vector %u at EIP %p, CS %04X, ErrorCode %u",
        static_cast<UInt32>(vector),
        context->EIP,
        context->CS,
        context->ErrorCode
      );

      // halt, unrecoverable
      while (true) {
        asm volatile(
          "cli\n"
          "hlt\n"
        );
      }
    }

    if (isIRQ) _controller->End(vector - IRQ_BASE_VECTOR);

    return static_cast<IA32InterruptContext*>(nextContext);
  }

  void IA32IDT::_clearEntriesAndHandlers() {
    for (UInt32 i = 0; i < IDT_ENTRY_COUNT; ++i) {
      _entries[i] = {};
      _handlerTable[i] = nullptr;
    }
  }

  void IA32IDT::_setGate(
    UInt8 vector,
    void (*handler)(),
    UInt8 typeAttribute
  ) {
    UInt32 address = reinterpret_cast<UInt32>(handler);
    IA32IDTEntry& entry = _entries[vector];

    entry.OffsetLow = address & 0xFFFF;
    entry.Selector = 0x08; // code segment selector
    entry.Zero = 0;
    entry.TypeAttribute = typeAttribute;
    entry.OffsetHigh = (address >> 16) & 0xFFFF;
  }

  IA32InterruptContext* IA32IDT::_dispatch(IA32InterruptContext& context) {
    if (_handlerTable[context.Vector] != nullptr) {
      return static_cast<IA32InterruptContext*>(
        _handlerTable[context.Vector](context)
      );
    } else if (_dispatcher) {
      return _dispatcher->Dispatch(context);
    } else {
      return nullptr;
    }
  }
}
