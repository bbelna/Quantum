/**
 * @file System/Kernel/IRQ.cpp
 * @brief IRQ handling and notification.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/IRQ.hpp>

#include "Arch/Interrupts.hpp"
#include "IRQ.hpp"
#include "IPC.hpp"
#include "Task.hpp"

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

    UInt8 vector = static_cast<UInt8>(32 + irq);

    Arch::Interrupts::RegisterHandler(vector, HandleIRQ);
    Arch::Interrupts::Unmask(static_cast<UInt8>(irq));

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

    Arch::Interrupts::Unmask(static_cast<UInt8>(irq));

    return true;
  }

  bool IRQ::Disable(UInt32 irq) {
    if (irq >= _maxIRQs) {
      return false;
    }

    Arch::Interrupts::Mask(static_cast<UInt8>(irq));

    return true;
  }

  Interrupts::Context* IRQ::HandleIRQ(Interrupts::Context& context) {
    UInt32 vector = context.vector;

    if (vector >= 32 && vector < 32 + _maxIRQs) {
      UInt32 irq = vector - 32;

      Notify(irq);
    }

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
