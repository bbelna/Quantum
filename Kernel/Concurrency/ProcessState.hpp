/**
 * @file Kernel/Concurrency/ProcessState.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ProcessState.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Possible states for a process.
   */
  enum class ProcessState : UInt8 {
    /**
     * @brief Process has been created but not yet started.
     */
    Created,

    /**
     * @brief Process is running (has at least one running thread).
     */
    Running,

    /**
     * @brief Process is stopped/suspended.
     */
    Stopped,

    /**
     * @brief Process has terminated and is awaiting cleanup.
     */
    Zombie,

    /**
     * @brief Process is being terminated.
     */
    Terminating
  };
}
