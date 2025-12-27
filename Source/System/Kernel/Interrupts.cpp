/**
 * @file System/Kernel/Interrupts.cpp
 * @brief Architecture-agnostic interrupt handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Interrupts.hpp"

namespace Quantum::System::Kernel {
  void Interrupts::Initialize() {
    Arch::Interrupts::Initialize();
  }

  void Interrupts::RegisterHandler(UInt8 vector, Interrupts::Handler handler) {
    Arch::Interrupts::RegisterHandler(vector, handler);
  }

  void Interrupts::End(UInt8 irq) {
    Arch::Interrupts::End(irq);
  }

  void Interrupts::Mask(UInt8 irq) {
    Arch::Interrupts::Mask(irq);
  }

  void Interrupts::MaskAll() {
    Arch::Interrupts::MaskAll();
  }

  void Interrupts::Unmask(UInt8 irq) {
    Arch::Interrupts::Unmask(irq);
  }

  void Interrupts::UnmaskAll() {
    Arch::Interrupts::UnmaskAll();
  }
}
