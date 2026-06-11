/**
 * @file Kernel/Concurrency/Spinlock.hpp
 * @brief Declares @ref @QKrnl::Concurrency::Spinlock.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/Atomic.hpp>
#include <Arch/InterruptControl.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Non-reentrant spinlock using architecture-provided atomics
   *        and interrupt control.
   * @tparam LockStateType The unsigned integer type used for the lock word
   *                       (typically @ref UInt32).
   *
   * @ref Acquire disables interrupts before spinning, which prevents
   * deadlocks when the same lock may be taken by both thread code and
   * interrupt handlers.  The saved interrupt flags are restored by
   * @ref Release.
   *
   * The lock is NOT reentrant: acquiring it twice on the same CPU without
   * an intervening release will deadlock.
   *
   * Architecture portability is provided by @ref IAtomic (for the lock
   * word) and @ref IInterruptControl (for interrupt save/restore and
   * spin hints).
   */
  template <typename LockStateType>
  class Spinlock {
    public:
      /**
       * @brief Creates a new @ref Spinlock (in the unlocked state).
       */
      explicit Spinlock() :
        _state(Arch::AtomicT<LockStateType>(0)),
        _savedFlags(0)
      {
      }

      /**
       * @brief Acquires the lock, spinning until available.
       *
       * Disables interrupts before acquiring to prevent deadlock if an
       * interrupt handler tries to acquire the same lock.
       */
      inline void Acquire() {
        auto flags = Arch::InterruptControl::SaveAndDisable();

        while (_state.Exchange(1) != 0) {
          Arch::InterruptControl::SpinHint();
        }

        _savedFlags = flags;
      }

      /**
       * @brief Releases the lock.
       *
       * Restores the interrupt state that was saved during @ref Acquire.
       */
      inline void Release() {
        auto flags = _savedFlags;

        // release the lock
        _state.Store(0);

        Arch::InterruptControl::Restore(flags);
      }

      /**
       * @brief Attempts to acquire the lock once without spinning.
       * @return `true` if the lock was acquired; `false` otherwise.
       * @note **Does not disable interrupts.** Caller must handle interrupt
       *       safety if needed.
       */
      inline bool TryAcquire() {
        return _state.Exchange(1) == 0;
      }

      /**
       * @brief Attempts to acquire the lock with interrupts disabled.
       * @return `true` if the lock was acquired; `false` otherwise.
       * @note If successful, interrupts are disabled and must be restored
       *       by calling @ref Release.
       */
      inline bool TryAcquireIRQ() {
        auto flags = Arch::InterruptControl::SaveAndDisable();

        if (_state.Exchange(1) == 0) {
          _savedFlags = flags;

          return true;
        }

        Arch::InterruptControl::Restore(flags);

        return false;
      }

    private:
      /**
       * @brief Lock state (`0` = unlocked, `1` = locked).
       */
      Arch::AtomicT<LockStateType> _state;

      /**
       * @brief Saved interrupt flags to restore on @ref Release.
       */
      typename Arch::InterruptControl::Flags _savedFlags;
  };
}
