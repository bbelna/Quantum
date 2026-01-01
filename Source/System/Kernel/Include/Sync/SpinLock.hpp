/**
 * @file System/Kernel/Include/Sync/SpinLock.hpp
 * @brief Spinlock synchronization primitive.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Arch/SpinLock.hpp"

namespace Quantum::System::Kernel::Sync {
  /**
   * Arch-agnostic spinlock wrapper.
   */
  class SpinLock {
    public:
      /**
       * Initializes the lock to unlocked state.
       */
      void Initialize() {
        _lock.Initialize();
      }

      /**
       * Acquires the lock, spinning until available.
       */
      void Acquire() {
        _lock.Acquire();
      }

      /**
       * Releases the lock.
       */
      void Release() {
        _lock.Release();
      }

      /**
       * Attempts to acquire the lock once.
       * @return
       *   True if the lock was acquired.
       */
      bool TryAcquire() {
        return _lock.TryAcquire();
      }

      /**
       * Acquires the lock with interrupts disabled.
       * @param flags
       *   Receives the previous interrupt flags.
       */
      void AcquireIRQSave(UInt32& flags) {
        _lock.AcquireIRQSave(flags);
      }

      /**
       * Releases the lock and restores interrupt flags.
       * @param flags
       *   Previous interrupt flags.
       */
      void ReleaseIRQRestore(UInt32 flags) {
        _lock.ReleaseIRQRestore(flags);
      }

    private:
      /**
       * Architecture-specific spinlock.
       */
      Arch::SpinLock _lock;
  };
}
