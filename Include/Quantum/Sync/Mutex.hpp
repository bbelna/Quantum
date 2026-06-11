/**
 * @file Include/Quantum/Sync/Mutex.hpp
 * @brief Futex-based mutual exclusion primitive.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Kernel.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Sync {
  /**
   * @brief Futex-based mutex.
   * 
   * Uncontended lock/unlock require zero syscalls. When contended, the
   * blocking thread sleeps via the kernel futex rather than spinning, freeing
   * the CPU for other work.
   *
   * Three-state design (Drepper, "Futexes Are Tricky"): 0 = unlocked,
   * 1 = locked (no waiters), 2 = locked (one or more waiters)
   */
  class Mutex {
    public:
      /**
       * @brief Creates a new `Mutex` instance.
       */
      Mutex() = default;

      /**
       * @brief Locks the mutex, blocking if already locked.
       */
      void Lock() {
        // fast path: uncontended, one cmpxchg, zero syscalls
        UInt32 c = __sync_val_compare_and_swap(&_state, 0, 1);

        if (c == 0) return;

        // slow path: contended, mark waiters and sleep
        if (c != 2)
          c = __sync_lock_test_and_set(&_state, 2);

        while (c != 0) {
          Kernel::ABI::Thread::FutexWait(&_state, 2);

          c = __sync_lock_test_and_set(&_state, 2);
        }
      }

      /**
       * @brief Unlocks the mutex, waking one waiter if present.
       */
      void Unlock() {
        // fast path: no waiters, one atomic sub, zero syscalls
        if (__sync_fetch_and_sub(&_state, 1) != 1) {
          // there were waiters, fully unlock and wake one
          __sync_lock_release(&_state);

          Kernel::ABI::Thread::FutexWake(&_state, 1);
        }
      }

    private:
      /**
       * @brief Mutex state: 0 = unlocked, 1 = locked (no waiters), 2 = locked
       *        (one or more waiters).
       */
      volatile UInt32 _state = 0;
  };
}
