/**
 * @file System/Kernel/Include/Arch/Task.hpp
 * @brief Architecture-specific task wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/Task.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchTask = KernelIA32::Task;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific task implementation.
   */
  using Task = ArchTask;
}
