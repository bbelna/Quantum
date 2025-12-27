/**
 * @file System/Kernel/Interrupts.cpp
 * @brief Architecture-agnostic interrupt handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Interrupts.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Interrupts.hpp"
#endif

namespace Quantum::System::Kernel {
  #if defined(QUANTUM_ARCH_IA32)
  using ArchInterrupts = Arch::IA32::Interrupts;
  #endif

  void Interrupts::Initialize() {
    ArchInterrupts::Initialize();
  }

  void Interrupts::RegisterHandler(UInt8 vector, Interrupts::Handler handler) {
    ArchInterrupts::RegisterHandler(vector, handler);
  }
}
