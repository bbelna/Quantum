/**
 * @file System/Kernel/Include/Arch/UserMode.hpp
 * @brief Architecture-specific user mode wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/Prelude.hpp"
#include "Arch/IA32/UserMode.hpp"

using ArchUserMode = KernelIA32::UserMode;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific user mode implementation.
   */
  using UserMode = ArchUserMode;
}
