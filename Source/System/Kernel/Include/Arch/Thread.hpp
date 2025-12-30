/**
 * @file System/Kernel/Include/Arch/Thread.hpp
 * @brief Architecture-specific thread wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#if defined(QUANTUM_ARCH_AMD64)
#else
#include "Arch/IA32/Thread.hpp"
#include "Arch/IA32/Prelude.hpp"

using ArchThread = KernelIA32::Thread;
#endif

namespace Quantum::System::Kernel::Arch {
  /**
   * Alias for the architecture-specific thread implementation.
   */
  using Thread = ArchThread;
}
