/**
 * @file System/Kernel/Include/Arch/SpinLock.hpp
 * @brief Architecture-specific spinlock wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/SpinLock.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchSpinLock = KernelIA32::SpinLock;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific spinlock implementation.
   */
  using SpinLock = ArchSpinLock;
}
