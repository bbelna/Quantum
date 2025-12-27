/**
 * @file System/Kernel/Include/Arch/Interrupts.hpp
 * @brief Architecture-specific interrupt handling wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchInterrupts = KernelIA32::Interrupts;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific interrupt implementation.
   */
  using Interrupts = ArchInterrupts;
}
