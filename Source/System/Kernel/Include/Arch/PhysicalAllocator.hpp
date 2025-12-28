/**
 * @file System/Kernel/Include/Arch/PhysicalAllocator.hpp
 * @brief Architecture-specific physical allocator wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/PhysicalAllocator.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchPhysicalAllocator = KernelIA32::PhysicalAllocator;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific physical allocator.
   */
  using PhysicalAllocator = ArchPhysicalAllocator;
}
