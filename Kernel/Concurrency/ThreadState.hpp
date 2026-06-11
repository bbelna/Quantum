/**
 * @file Kernel/Concurrency/ThreadState.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ThreadState.
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
   * @brief @ref Thread runtime states.
   *
   * State transitions are managed exclusively by @ref ThreadManager.
   */
  enum class ThreadState : UInt8 {
    /**
     * @brief @ref Thread has been created but not yet started.
     */
    Created,

    /**
     * @brief @ref Thread is ready to run and waiting in the scheduler queue.
     */
    Ready,

    /**
     * @brief @ref Thread is currently executing on a CPU.
     */
    Running,

    /**
     * @brief @ref Thread is sleeping for (up to) a specified duration.
     */
    Sleeping,

    /**
     * @brief @ref Thread has been suspended and will not run until resumed.
     */
    Suspended,

    /**
     * @brief @ref Thread has finished execution and is awaiting cleanup.
     */
    Terminated
  };
}
