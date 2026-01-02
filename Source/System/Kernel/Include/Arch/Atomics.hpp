/**
 * @file System/Kernel/Include/Arch/Atomics.hpp
 * @brief Architecture-specific atomics wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/Atomics.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchAtomics = KernelIA32::Atomics;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific atomics implementation.
   */
  using Atomics = ArchAtomics;
}
