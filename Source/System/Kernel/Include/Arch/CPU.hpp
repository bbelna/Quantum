/**
 * @file System/Kernel/Include/Arch/CPU.hpp
 * @brief Architecture-specific CPU wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchCPU = KernelIA32::CPU;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific CPU implementation.
   */
  using CPU = ArchCPU;
}
