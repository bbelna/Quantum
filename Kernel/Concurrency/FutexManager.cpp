/**
 * @file Kernel/Concurrency/FutexManager.cpp
 * @brief Implements @ref @QKrnl::Concurrency::FutexManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FutexManager.hpp"
#include "ThreadManager.hpp"

namespace Quantum::Kernel::Concurrency {
  bool FutexManager::Wait(
    UIntPtr address,
    UInt32 expected,
    Thread* thread
  ) {
    _lock.Acquire();

    // atomically check the value while holding the lock
    // prevents lost wakes where Wake fires between the userspace
    // compare and the kernel suspend
    volatile UInt32* ptr = reinterpret_cast<volatile UInt32*>(address);

    // value doesn't match, don't suspend
    if (*ptr != expected) {
      _lock.Release();

      return false;
    }

    // add thread to the wait queue for this address;
    // the caller is responsible for blocking the thread (via Sleep)
    Size bucket = _hash(address);

    thread->FutexWaitAddress = address;
    thread->FutexNext = _buckets[bucket];
    _buckets[bucket] = thread;

    _lock.Release();

    return true;
  }

  Size FutexManager::Wake(UIntPtr address, Size maxCount) {
    // collect threads to wake while holding the lock, then resume them
    // after releasing, calling Resume under the spinlock risks deadlock
    // if the woken thread immediately needs the futex manager
    Thread* threadsToWake[THREAD_DEFERRED_WAKE_LIMIT];
    Size awoken = 0;

    _lock.Acquire();

    Size bucket = _hash(address);
    Thread* previous = nullptr;
    Thread* current = _buckets[bucket];

    // walk the wait queue for this bucket and collect threads waiting on
    // the address until we hit maxCount or the end of the queue
    while (
      current &&
      awoken < maxCount
    ) {
      Thread* next = current->FutexNext;

      if (current->FutexWaitAddress == address) {
        if (previous) {
          previous->FutexNext = next;
        } else {
          _buckets[bucket] = next;
        }

        current->FutexWaitAddress = 0;
        current->FutexNext = nullptr;

        if (awoken < THREAD_DEFERRED_WAKE_LIMIT) {
          threadsToWake[awoken] = current;
        }

        awoken++;
      } else {
        previous = current;
      }

      current = next;
    }

    _lock.Release();

    // resume collected threads now that the lock is released
    Size resumeCount
      = awoken < THREAD_DEFERRED_WAKE_LIMIT
      ? awoken
      : THREAD_DEFERRED_WAKE_LIMIT;

    for (
      Size threadIndex = 0;
      threadIndex < resumeCount;
      threadIndex++
    ) {
      _threadManager.Resume(threadsToWake[threadIndex]);
    }

    return awoken;
  }
}
