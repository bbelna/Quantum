/**
 * @file Kernel/Concurrency/ThreadFlags.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ThreadFlags.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Bitmask flags for thread creation and runtime behavior.
   *
   * Combined with bitwise OR via @ref QUANTUM_ENABLE_ENUM_BITMASK_OPS.
   */
  enum class ThreadFlags : UInt32 {
    /**
     * @brief No special flags.
     */
    None = 0,

    /**
     * @brief Thread runs in kernel mode.
     */
    Kernel = 1 << 0,

    /**
     * @brief Thread runs in user mode.
     */
    User = 1 << 1,

    /**
     * @brief Thread is detached and will clean up automatically on exit.
     */
    Detached = 1 << 2,

    /**
     * @brief Thread is joinable and another thread can wait for it.
     */
    Joinable = 1 << 3
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::Kernel::Concurrency, ThreadFlags)
