/**
 * @file System/Kernel/Include/Sync/ScopedIRQLock.hpp
 * @brief Scoped IRQ lock guard helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Sync {
  /**
   * Scoped lock guard for irqsave/irqrestore spinlocks.
   */
  template <typename LockType>
  class ScopedIRQLock {
    public:
      /**
       * Constructs a scoped irq lock guard and acquires the lock.
       * @param lock
       *   Lock to acquire.
       */
      explicit ScopedIRQLock(LockType& lock) : _lock(lock) {
        _lock.AcquireIRQSave(_flags);
      }

      /**
       * Destructs the scoped irq lock guard and releases the lock.
       */
      ~ScopedIRQLock() {
        _lock.ReleaseIRQRestore(_flags);
      }

      /**
       * Deleted copy constructor.
       */
      ScopedIRQLock(const ScopedIRQLock&) = delete;

      /**
       * Deleted copy assignment operator.
       */
      ScopedIRQLock& operator=(const ScopedIRQLock&) = delete;

    private:
      /**
       * Lock reference.
       */
      LockType& _lock;

      /**
       * Saved flags.
       */
      UInt32 _flags = 0;
  };
}
