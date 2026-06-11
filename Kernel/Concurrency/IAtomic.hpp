/**
 * @file Kernel/Concurrency/IAtomic.hpp
 * @brief Declares @ref @QKrnl::Concurrency::IAtomic.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "AtomicMemoryOrder.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief CRTP interface for architecture-specific atomic operations.
   * @tparam ValueType The type to be made atomic.
   * @tparam Impl The concrete architecture-specific implementation
   *              (e.g., @ref IA32Atomic).
   *
   * Provides a uniform public API for atomic load, store, exchange,
   * compare-exchange, and fetch-add operations. The implementation is
   * resolved entirely at compile time through the CRTP, so there is no
   * virtual dispatch overhead.
   *
   * Architecture implementations must provide the following private
   * methods (accessible via `friend`):
   *
   * - `ValueType DoLoad(AtomicMemoryOrder) const`
   * - `void DoStore(ValueType, AtomicMemoryOrder)`
   * - `ValueType DoExchange(ValueType, AtomicMemoryOrder)`
   * - `bool DoCompareExchange(ValueType&, ValueType,
   *        AtomicMemoryOrder, AtomicMemoryOrder)`
   * - `ValueType DoFetchAdd(ValueType, AtomicMemoryOrder)`
   * - `bool DoIsLockFree() const`
   */
  template <typename ValueType, typename Impl>
  class IAtomic {
    public:
      IAtomic(const IAtomic&) = delete;
      IAtomic& operator=(const IAtomic&) = delete;

      /**
       * @brief Loads the atomic value with the specified memory order.
       * @param order The memory order for the load operation.
       * @return The loaded value.
       */
      ValueType Load(
        AtomicMemoryOrder order = AtomicMemoryOrder::SequentiallyConsistent
      ) const {
        return impl()
          .DoLoad(order);
      }

      /**
       * @brief Stores a value into the atomic with the specified
       *        @ref AtomicMemoryOrder.
       * @param value The value to store.
       * @param order The @ref AtomicMemoryOrder for the store operation.
       */
      void Store(
        ValueType value,
        AtomicMemoryOrder order = AtomicMemoryOrder::SequentiallyConsistent
      ) {
        impl()
         .DoStore(
            value,
            order
          );
      }

      /**
       * @brief Atomically exchanges the atomic value with the given value.
       * @param value The value to exchange.
       * @param order The @ref AtomicMemoryOrder for the exchange operation.
       * @return The old value before the exchange.
       */
      ValueType Exchange(
        ValueType value,
        AtomicMemoryOrder order = AtomicMemoryOrder::SequentiallyConsistent
      ) {
        return impl()
          .DoExchange(
            value,
            order
          );
      }

      /**
       * @brief Atomically compares and exchanges the atomic value.
       * @param expected Reference to the expected value. Updated with the
       *                 actual value if the exchange fails.
       * @param desired The value to set if the current value matches
       *                @p expected.
       * @param successOrder The @ref AtomicMemoryOrder for a successful
       *                     exchange.
       * @param failureOrder The @ref AtomicMemoryOrder for a failed exchange.
       * @return `true` if the exchange was successful; `false` otherwise.
       */
      bool CompareExchange(
        ValueType& expected,
        ValueType desired,
        AtomicMemoryOrder successOrder
          = AtomicMemoryOrder::SequentiallyConsistent,
        AtomicMemoryOrder failureOrder
          = AtomicMemoryOrder::SequentiallyConsistent
      ) {
        return impl()
          .DoCompareExchange(
            expected,
            desired,
            successOrder,
            failureOrder
          );
      }

      /**
       * @brief Atomically adds a delta to the atomic value.
       * @param delta The value to add.
       * @param order The @ref AtomicMemoryOrder for the addition operation.
       * @return The old value before the addition.
       */
      ValueType FetchAdd(
        ValueType delta,
        AtomicMemoryOrder order = AtomicMemoryOrder::SequentiallyConsistent
      ) {
        return  impl()
          .DoFetchAdd(
            delta,
            order
          );
      }

      /**
       * @brief Checks if operations on this atomic type are lock-free.
       * @return `true` if lock-free; `false` otherwise.
       */
      bool IsLockFree() const {
        return  impl()
          .DoIsLockFree();
      }

    protected:
      IAtomic() = default;

    private:
      Impl& impl() {
        return static_cast<Impl&>(*this);
      }

      const Impl& impl() const {
        return static_cast<const Impl&>(*this);
      }
  };
}
