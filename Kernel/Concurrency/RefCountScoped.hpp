/**
 * @file Kernel/Concurrency/RefCountScoped.hpp
 * @brief Declares @ref @QKrnl::Concurrency::RefCountScoped.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "RefCount.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Reference-counted object base class that automatically deletes
   *        itself when the reference count reaches zero.
   *
   * Inherit from this class (with `protected` inheritance) to give an
   * object automatic-lifetime semantics.  Only the thread that transitions
   * the count from 1 to 0 will invoke `delete this`; all other threads
   * see a non-zero return from @ref DecrementRefCount.
   *
   * The object must have been allocated with `new` (kernel heap) because
   * the destructor path calls `delete this`.
   */
  class RefCountScoped : protected RefCount {
    public:
      /**
       * @brief Creates a new `RefCountScoped` with the given initial count.
       * @param initialCount The initial reference count value.
       */
      explicit RefCountScoped(UInt32 initialCount = 0)
        : RefCount(initialCount) {}

      /**
       * @brief Destructs this `RefCountScoped` instance.
       */
      virtual ~RefCountScoped() = default;

      /**
       * @brief Increments the reference count.
       * @return New reference count value.
       */
      UInt32 IncrementRefCount() { return RefCount::Increment(); }

      /**
       * @brief Decrements the reference count and deletes the object if the
       *        count reaches zero.
       * @return The new reference count value.  If zero, `this` has already
       *         been deleted and the caller must not dereference the object.
       *
       * Uses a single atomic fetch-add to avoid a TOCTOU race: only the
       * thread that observes old-count == 1 will call `delete this`.
       * If the count was already zero (underflow), the decrement is rolled
       * back to prevent wraparound.
       */
      UInt32 DecrementRefCount() {
        // FetchAdd returns the OLD value atomically; only the thread that
        // transitions 1 -> 0 will see oldCount == 1 and delete this,
        // eliminating the TOCTOU race in the previous read-then-decrement
        UInt32 oldCount = _count.FetchAdd(static_cast<UInt32>(-1));

        if (oldCount == 0) {
          // was already zero, restore to prevent underflow
          _count.FetchAdd(1);
          return 0;
        }

        UInt32 newCount = oldCount - 1;

        if (newCount == 0) delete this;

        return newCount;
      }

      /**
       * @brief Gets the current reference count.
       * @return Current reference count value.
       */
      UInt32 GetRefCount() const { return RefCount::Get(); }
  };
}
