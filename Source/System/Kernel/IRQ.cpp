/**
 * @file System/Kernel/IRQ.cpp
 * @brief IRQ handling and notification.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/IRQ.hpp>

#include "IRQ.hpp"
#include "IPC.hpp"
#include "Task.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/PIC.hpp"
#endif

namespace Quantum::System::Kernel {
  using IPC = Kernel::IPC;
  using Task = Kernel::Task;

  bool IRQ::Register(UInt32 irq, UInt32 portId) {
    if (irq >= _maxIRQs || portId == 0) {
      return false;
    }

    UInt32 ownerId = 0;

    if (!IPC::GetPortOwner(portId, ownerId)) {
      return false;
    }

    bool isCoordinator = Task::IsCurrentTaskCoordinator();

    if (!isCoordinator && ownerId != Task::GetCurrentId()) {
      return false;
    }

    _irqPorts[irq] = portId;

    #if defined(QUANTUM_ARCH_IA32)
    UInt8 vector = static_cast<UInt8>(32 + irq);

    Arch::IA32::Interrupts::RegisterHandler(vector, HandleIRQ);
    Arch::IA32::PIC::Unmask(static_cast<UInt8>(irq));
    #endif

    return true;
  }

  bool IRQ::Unregister(UInt32 irq) {
    if (irq >= _maxIRQs) {
      return false;
    }

    _irqPorts[irq] = 0;

    return true;
  }

  bool IRQ::Enable(UInt32 irq) {
    if (irq >= _maxIRQs) {
      return false;
    }

    #if defined(QUANTUM_ARCH_IA32)
    Arch::IA32::PIC::Unmask(static_cast<UInt8>(irq));
    #endif

    return true;
  }

  bool IRQ::Disable(UInt32 irq) {
    if (irq >= _maxIRQs) {
      return false;
    }

    #if defined(QUANTUM_ARCH_IA32)
    Arch::IA32::PIC::Mask(static_cast<UInt8>(irq));
    #endif

    return true;
  }

  Interrupts::Context* IRQ::HandleIRQ(Interrupts::Context& context) {
    UInt32 vector = context.vector;

    #if defined(QUANTUM_ARCH_IA32)
    if (vector >= 32 && vector < 32 + _maxIRQs) {
      UInt32 irq = vector - 32;

      Notify(irq);
    }
    #else
    (void)vector;
    #endif

    return &context;
  }

  void IRQ::Notify(UInt32 irq) {
    if (irq >= _maxIRQs) {
      return;
    }

    UInt32 portId = _irqPorts[irq];

    if (portId == 0) {
      return;
    }

    ABI::IRQ::Message payload {};

    payload.op = 0;
    payload.irq = irq;
    payload.portId = 0;
    payload.replyPortId = 0;
    payload.data = 0;

    IPC::Send(portId, Task::GetCurrentId(), &payload, sizeof(payload));
  }
}
