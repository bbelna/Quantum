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
  template <typename LockT>
  class ScopedLock {
    public:
      explicit ScopedLock(LockT& lock) : _lock(lock) {
        _lock.Acquire();
      }

      ~ScopedLock() {
        _lock.Release();
      }

      ScopedLock(const ScopedLock&) = delete;
      ScopedLock& operator=(const ScopedLock&) = delete;

    private:
      LockT& _lock;
  };

  /**
   * Scoped lock guard for irqsave/irqrestore spinlocks.
   */
  template <typename LockT>
  class ScopedIrqLock {
    public:
      explicit ScopedIrqLock(LockT& lock) : _lock(lock) {
        _lock.AcquireIrqSave(_flags);
      }

      ~ScopedIrqLock() {
        _lock.ReleaseIrqRestore(_flags);
      }

      ScopedIrqLock(const ScopedIrqLock&) = delete;
      ScopedIrqLock& operator=(const ScopedIrqLock&) = delete;

    private:
      LockT& _lock;
      UInt32 _flags = 0;
  };
}
