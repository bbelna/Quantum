/**
 * @file System/Kernel/Include/Arch/Memory.hpp
 * @brief Architecture-specific memory wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Memory.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchMemory = KernelIA32::Memory;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific memory implementation.
   */
  using Memory = ArchMemory;
}
