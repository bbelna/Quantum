/**
 * @file System/Kernel/CPU.cpp
 * @brief Architecture-agnostic CPU handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/CPU.hpp"
#include "CPU.hpp"

namespace Quantum::System::Kernel {
  void CPU::HaltForever() {
    Arch::CPU::HaltForever();
  }

  void CPU::Pause() {
    Arch::CPU::Pause();
  }

  void CPU::DisableInterrupts() {
    Arch::CPU::DisableInterrupts();
  }

  void CPU::EnableInterrupts() {
    Arch::CPU::EnableInterrupts();
  }
}
