/**
 * @file System/Kernel/CPU.cpp
 * @brief Architecture-agnostic CPU handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "CPU.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/CPU.hpp"

namespace Arch = Quantum::System::Kernel::Arch::IA32;
using ArchCPU = Arch::CPU;
#endif

namespace Quantum::System::Kernel {
  void CPU::HaltForever() {
    ArchCPU::HaltForever();
  }

  void CPU::Pause() {
    ArchCPU::Pause();
  }

  void CPU::DisableInterrupts() {
    ArchCPU::DisableInterrupts();
  }

  void CPU::EnableInterrupts() {
    ArchCPU::EnableInterrupts();
  }
}
