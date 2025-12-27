/**
 * @file System/Kernel/Include/Arch/CPU.hpp
 * @brief Architecture-specific boot info wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/BootInfo.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchBootInfo = KernelIA32::BootInfo;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific boot info implementation.
   */
  using BootInfo = ArchBootInfo;
}
