/**
 * @file Kernel/Concurrency/ThreadWaitNode.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ThreadWaitNode.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Intrusive node for @ref Thread wait queues.
   *
   * Embeddable in @ref Thread or allocatable on the stack for multi-port waits.
   * Contains @ref ThreadWaitNode::Next / @ref ThreadWaitNode::Previous pointers
   * for use with @ref IntrusiveQueue.
   */
  struct ThreadWaitNode {
    /**
     * @brief The @ref ThreadWaitNode following this node in the wait queue, or
     *        `nullptr` if this is the tail.
     */
    ThreadWaitNode* Next = nullptr;

    /**
     * @brief Previous @ref ThreadWaitNode in the wait queue, or `nullptr` if
     *        this is the head.
     */
    ThreadWaitNode* Previous = nullptr;

    /**
     * @brief The @ref Thread instance that is waiting on this node.
     */
    Thread* Owner = nullptr;
  };
}
