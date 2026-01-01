/**
 * @file System/Kernel/Include/Arch/IA32/SpinLock.hpp
 * @brief IA32 spinlock implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Atomics.hpp>
#include <Types.hpp>

#include "CPU.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 spinlock (non-recursive).
   */
  class SpinLock {
    public:
      /**
       * Initializes the lock to unlocked state.
       */
      void Initialize() {
        _state.Store(0);
      }

      /**
       * Acquires the lock, spinning until available.
       */
      void Acquire() {
        while (_state.Exchange(1) != 0) {
          CPU::Pause();
        }
      }

      /**
       * Releases the lock.
       */
      void Release() {
        _state.Store(0);
      }

      /**
       * Attempts to acquire the lock once.
       * @return
       *   True if the lock was acquired.
       */
      bool TryAcquire() {
        return _state.Exchange(1) == 0;
      }

      /**
       * Acquires the lock with interrupts disabled.
       * @param flags
       *   Receives the previous interrupt flags.
       */
      void AcquireIRQSave(UInt32& flags) {
        asm volatile(
          "pushf\n"
          "pop %0\n"
          "cli\n"
          : "=r"(flags)
          :
          : "memory"
        );

        Acquire();
      }

      /**
       * Releases the lock and restores interrupt flags.
       * @param flags
       *   Previous interrupt flags.
       */
      void ReleaseIRQRestore(UInt32 flags) {
        Release();

        asm volatile(
          "push %0\n"
          "popf\n"
          :
          : "r"(flags)
          : "memory"
        );
      }

    private:
      /**
       * Lock state (0 = unlocked, 1 = locked).
       */
      Kernel::Atomic<UInt32> _state;
  };
}
