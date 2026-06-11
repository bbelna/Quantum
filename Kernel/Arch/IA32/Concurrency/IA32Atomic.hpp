/**
 * @file Kernel/Arch/IA32/Concurrency/IA32Atomic.hpp
 * @brief Declares @ref @QKrnlIA32::Concurrency::IA32Atomic.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/IAtomic.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief IA-32 implementation of @ref IAtomic for trivially copyable
   *        types.
   * @tparam ValueType
   *   The type to be made atomic. Must be trivially copyable and of size
   *   1, 2, 4, or 8 bytes.
   *
   * On IA-32, 1/2/4-byte operations are naturally atomic when aligned
   * and use `lock`-prefixed instructions for read-modify-write. 8-byte
   * operations use `lock cmpxchg8b`. The underlying value is aligned to
   * its own size to guarantee atomicity without bus locks.
   */
  template <typename ValueType>
  class IA32Atomic : public IAtomic<ValueType, IA32Atomic<ValueType>> {
    friend class IAtomic<ValueType, IA32Atomic<ValueType>>;

    static_assert(
      IS_TRIVIALLY_COPYABLE(ValueType),
      "Atomic<T> requires trivially copyable type"
    );
    static_assert(
      sizeof(ValueType) == 1 ||
      sizeof(ValueType) == 2 ||
      sizeof(ValueType) == 4 ||
      sizeof(ValueType) == 8,
      "Atomic<T> supports 1/2/4/8 byte types"
    );

  public:
    /**
     * @brief Creates a new `IA32Atomic` with default-initialized value.
     */
    constexpr IA32Atomic() : _value() {}

    /**
     * @brief Creates a new `IA32Atomic` with the given initial value.
     * @param value The initial value.
     */
    constexpr explicit IA32Atomic(ValueType value) : _value(value) {}

    /**
     * @brief Returns a pointer to the underlying atomic value.
     * @return Pointer to the atomic value.
     */
    inline volatile ValueType* RawPtr() { return &_value; }

    /**
     * @brief Returns a `const` pointer to the underlying atomic value.
     * @return Const pointer to the atomic value.
     */
    inline const volatile ValueType* RawPtr() const { return &_value; }

  private:
    /**
     * @brief Loads the atomic value with the specified memory order.
     * @param order The memory order for the load operation.
     * @return The loaded value.
     */
    ValueType DoLoad(AtomicMemoryOrder order) const;

    /**
     * @brief Stores a value into the atomic with the specified memory
     *        order.
     * @param value The value to store.
     * @param order The memory order for the store operation.
     */
    void DoStore(ValueType value, AtomicMemoryOrder order);

    /**
     * @brief Atomically exchanges the atomic value with the given value.
     * @param value The value to exchange.
     * @param order The memory order for the exchange operation.
     * @return The old value before the exchange.
     */
    ValueType DoExchange(ValueType value, AtomicMemoryOrder order);

    /**
     * @brief Atomically compares and exchanges the atomic value.
     * @param expected Reference to the expected value. Updated with the
     *                 actual value if the exchange fails.
     * @param desired The value to set if the current value matches
     *                @p expected.
     * @param successOrder The memory order for a successful exchange.
     * @param failureOrder The memory order for a failed exchange.
     * @return `true` if the exchange was successful; `false` otherwise.
     */
    bool DoCompareExchange(
      ValueType& expected,
      ValueType desired,
      AtomicMemoryOrder successOrder,
      AtomicMemoryOrder failureOrder
    );

    /**
     * @brief Atomically adds a delta to the atomic value.
     * @param delta The value to add.
     * @param order The memory order for the addition operation.
     * @return The old value before the addition.
     */
    ValueType DoFetchAdd(ValueType delta, AtomicMemoryOrder order);

    /**
     * @brief Checks if operations on this atomic type are lock-free.
     * @return `true` if lock-free; `false` otherwise.
     */
    bool DoIsLockFree() const;

    /**
     * @brief The underlying atomic value.
     */
    alignas(sizeof(ValueType)) volatile ValueType _value;
  };

  /**
   * @brief Architecture-agnostic alias for @ref IA32Atomic.
   */
  template <typename ValueType>
  using Atomic = IA32Atomic<ValueType>;

  /**
   * @brief Inserts a compiler barrier to prevent instruction
   *        reordering.
   */
  inline void CompilerBarrier() {
    asm volatile("" ::: "memory");
  }

  /**
   * @brief Inserts a locked memory barrier to prevent memory
   *        operation reordering.
   */
  inline void LockedBarrier() {
    UInt32 dummy = 0;

    asm volatile(
      "lock\naddl $0, %0" : "+m"(dummy) :: "cc", "memory"
    );
  }
}
