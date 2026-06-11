/**
 * @file Kernel/Concurrency/FutexManager.hpp
 * @brief Declares @ref @QKrnl::Concurrency::FutexManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Spinlock.hpp"
#include "Thread.hpp"
#include "ThreadManager.hpp"
#include "ThreadWaitNode.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Manages futex wait queues.
   * @see <a href="https://en.wikipedia.org/wiki/Futex">Wikipedia: Futex</a>
   *
   * @ref Thread instances that call @ref FutexManager::Wait are suspended and
   * placed in a hash-bucketed wait queue keyed by address.
   *
   * @ref FutexManager::Wake scans the matching bucket and resumes up to `N`
   * waiters.
   *
   * The @ref Thread instance's @ref Thread::FutexWaitAddress and
   * @ref Thread::FutexNext fields provide intrusive linkage.
   */
  class FutexManager {
    public:
      /**
       * @brief
       *   Creates a new @ref FutexManager with empty wait queues.
       * @param threadManager
       *   Reference to the @ref ThreadManager, used to suspend and resume
       *   @ref Thread instances during @ref Wait and @ref Wake operations.
       */
      explicit FutexManager(ThreadManager& threadManager)
        : _threadManager(threadManager) {}

      /**
       * @brief Atomically checks `*address == expected` and, if `true`,
       *        suspends the calling @ref Thread on @p address.
       * @param address Process-space address of the @ref UInt32 futex word.
       * @param expected Value the caller expects at @p address.
       * @param thread The calling @ref Thread (will be suspended on match).
       * @return `true` if the @ref Thread was suspended (value matched),
       *         `false` if the value did not match (thread stays running).
       *
       * The check-and-suspend is performed under the bucket spinlock so
       * that a concurrent @ref Wake cannot be lost between the comparison
       * and the @ref Thread being enqueued. The @ref Thread is linked into the
       * bucket via @ref Thread::FutexWaitAddress and @ref Thread::FutexNext
       * (intrusive list, no heap allocation).
       */
      bool Wait(
        UIntPtr address,
        UInt32 expected,
        Thread* thread
      );

      /**
       * @brief Wakes up to @p maxCount @ref Thread waiting on @p address.
       * @param address The futex address whose waiters should be woken.
       * @param maxCount Maximum number of @ref Thread to wake. Pass
       *                 `static_cast<Size>(-1)` to wake all waiters.
       * @return The number of @ref Thread actually woken (may be less than
       *         @p maxCount if fewer threads were waiting).
       *
       * Woken @ref Thread are removed from the futex bucket and resumed via
       * @ref ThreadManager::Resume. The bucket @ref Spinlock (@ref _lock) is
       * held for the duration of the scan, so large @p maxCount values block
       * other futex operations on the same bucket.
       */
      Size Wake(UIntPtr address, Size maxCount);

    private:
      /**
       * @brief Reference to the @ref ThreadManager.
       */
      ThreadManager& _threadManager;

      /**
       * @brief Hash table buckets for futex wait queues.
       */
      Thread* _buckets[FUTEX_BUCKET_COUNT] = {};

      /**
       * @brief @ref Spinlock protecting the futex wait queues.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Hashes an address to a bucket index.
       * @param address The address to hash.
       * @return The bucket index for the address.
       */
      Size _hash(UIntPtr address) const {
        return (address >> 2) % FUTEX_BUCKET_COUNT;
      }
  };
}
