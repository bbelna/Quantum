/**
 * @file System/Kernel/Include/Arch/AddressSpace.hpp
 * @brief Architecture-specific address space wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/AddressSpace.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchAddressSpace = KernelIA32::AddressSpace;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific address space implementation.
   */
  using AddressSpace = ArchAddressSpace;
}
