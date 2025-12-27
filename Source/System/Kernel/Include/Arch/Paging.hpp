/**
 * @file System/Kernel/Include/Arch/Paging.hpp
 * @brief Architecture-specific paging wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/Paging.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchPaging = KernelIA32::Paging;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific paging implementation.
   */
  using Paging = ArchPaging;
}
