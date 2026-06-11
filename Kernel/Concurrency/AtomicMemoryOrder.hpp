/**
 * @file Kernel/Concurrency/AtomicMemoryOrder.hpp
 * @brief Declares @ref @QKrnl::Concurrency::AtomicMemoryOrder.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Memory ordering constraints for @ref IAtomic operations.
   */
  enum class AtomicMemoryOrder : UInt8 {
    /**
     * @brief No synchronization or ordering constraints.
     *
     * The compiler may reorder this operation freely with respect to other
     * memory accesses. Suitable for statistics counters and other cases
     * where only atomicity (not ordering) is required.
     */
    Relaxed,

    /**
     * @brief Acquire ordering: no reads or writes in the current thread
     *        can be reordered before this load.
     *
     * Typically paired with a @ref Release store in another thread to
     * establish a happens-before relationship (e.g. reading a spinlock).
     */
    Acquire,

    /**
     * @brief Release ordering: no reads or writes in the current thread
     *        can be reordered after this store.
     *
     * Ensures that all preceding writes are visible to a thread that
     * performs an @ref Acquire load on the same variable.
     */
    Release,

    /**
     * @brief Combined @ref Acquire and @ref Release semantics.
     *
     * Used for read-modify-write operations (e.g. compare-and-swap) that
     * must both observe prior writes and publish their own.
     */
    AcquireRelease,

    /**
     * @brief Sequentially consistent ordering.
     *
     * The strongest guarantee: establishes a single total order of all
     * sequentially-consistent operations across all threads. Implies
     * both @ref Acquire and @ref Release. Prefer weaker orders when the
     * total-order guarantee is not required.
     */
    SequentiallyConsistent
  };
}
