/**
 * @file Kernel/Concurrency/RefCount.hpp
 * @brief Declares @ref @QKrnl::Concurrency::RefCount.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include <Arch/Atomic.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Atomic reference counter with lock-free increment/decrement.
   *
   * All operations use relaxed atomic fetch-add, which is sufficient on
   * IA-32's strong memory model.  The counter does not perform any
   * automatic cleanup; see @ref RefCountScoped for self-deleting behavior.
   */
  class RefCount {
    public:
      /**
       * @brief Creates a new `RefCount` with the given initial count.
       * @param initialCount The initial reference count value.
       */
      explicit RefCount(UInt32 initialCount = 0) : _count(initialCount) {}

      /**
       * @brief Destructs this `RefCount` instance.
       */
      virtual ~RefCount() = default;

      /**
       * @brief Atomically increments the reference count.
       * @return The new reference count value (after increment).
       *
       * Lock-free; safe to call from any context including interrupt
       * handlers.
       */
      UInt32 Increment() { return _count.FetchAdd(1) + 1; }

      /**
       * @brief Atomically decrements the reference count.
       * @return The new reference count value (after decrement).
       *
       * The caller is responsible for acting on a zero return (e.g.
       * freeing the guarded resource).  Does not check for underflow;
       * decrementing a zero count is undefined behavior.
       */
      UInt32 Decrement() {
        return _count.FetchAdd(static_cast<UInt32>(-1)) - 1;
      }

      /**
       * @brief Reads the current reference count (snapshot).
       * @return Current reference count value.
       *
       * The returned value may be stale by the time the caller acts on it.
       * Use only for diagnostics or heuristics, never for synchronization
       * decisions.
       */
      UInt32 Get() const { return _count.Load(); }

    protected:
      /**
       * @brief Reference count value.
       */
      Arch::AtomicT<UInt32> _count;
  };
}
