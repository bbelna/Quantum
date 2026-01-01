/**
 * @file System/Kernel/Include/Sync/ScopedLock.hpp
 * @brief Scoped lock guard helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Sync {
  /**
   * Scoped lock guard for simple locks.
   */
  template <typename LockType>
  class ScopedLock {
    public:
      /**
       * Constructs a scoped lock guard and acquires the lock.
       * @param lock
       *   Lock to acquire.
       */
      explicit ScopedLock(LockType& lock) : _lock(lock) {
        _lock.Acquire();
      }

      /**
       * Destructs the scoped lock guard and releases the lock.
       */
      ~ScopedLock() {
        _lock.Release();
      }

      /**
       * Deleted copy constructor.
       */
      ScopedLock(const ScopedLock&) = delete;

      /**
       * Deleted copy assignment operator.
       */
      ScopedLock& operator=(const ScopedLock&) = delete;

    private:
      /**
       * Lock reference.
       */
      LockType& _lock;
  };
}
