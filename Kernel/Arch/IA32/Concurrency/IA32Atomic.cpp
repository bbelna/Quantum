/**
 * @file Kernel/Arch/IA32/Concurrency/IA32Atomic.cpp
 * @brief Implements @ref @QKrnlIA32::Concurrency::IA32Atomic.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "IA32Atomic.hpp"

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  template <typename ValueType>
  ValueType IA32Atomic<ValueType>::DoLoad(AtomicMemoryOrder order) const {
    ValueType value = *(RawPtr());

    if (
      order == AtomicMemoryOrder::Acquire ||
      order == AtomicMemoryOrder::AcquireRelease ||
      order == AtomicMemoryOrder::SequentiallyConsistent
    ) CompilerBarrier();

    return value;
  }

  template <typename ValueType>
  void IA32Atomic<ValueType>::DoStore(
    ValueType value,
    AtomicMemoryOrder order
  ) {
    if (
      order == AtomicMemoryOrder::Release ||
      order == AtomicMemoryOrder::AcquireRelease ||
      order == AtomicMemoryOrder::SequentiallyConsistent
    ) CompilerBarrier();

    *(&_value) = value;

    if (order == AtomicMemoryOrder::SequentiallyConsistent) {
      LockedBarrier();
    }
  }

  template <typename ValueType>
  ValueType IA32Atomic<ValueType>::DoExchange(
    ValueType value,
    AtomicMemoryOrder order
  ) {
    (void)order;

    if constexpr (sizeof(ValueType) == 4) {
      UInt32 val = (UInt32)value;

      asm volatile(
        "xchgl %0, %1"
        : "+r"(val), "+m"(*(volatile UInt32*)RawPtr())
        :
        : "memory"
      );

      return (ValueType)val;
    } else {
      static_assert(
        sizeof(ValueType) == 4,
        "exchange: implement other widths as needed"
      );
    }
  }

  template <typename ValueType>
  bool IA32Atomic<ValueType>::DoCompareExchange(
    ValueType& expected,
    ValueType desired,
    AtomicMemoryOrder successOrder,
    AtomicMemoryOrder failureOrder
  ) {
    (void)successOrder;
    (void)failureOrder;

    if constexpr (sizeof(ValueType) == 4) {
      UInt32 expected32 = (UInt32)expected;
      UInt32 previous;

      asm volatile(
        "lock\n"
        "cmpxchgl %3, %1"
        : "=a"(previous), "+m"(*(volatile UInt32*)RawPtr())
        : "a"(expected32), "r"((UInt32)desired)
        : "cc", "memory"
      );

      bool ok = previous == expected32;

      expected = (ValueType)previous;

      return ok;
    } else if constexpr (sizeof(ValueType) == 8) {
      UInt64 expected64 = (UInt64)expected;
      UInt32 expectedLow = (UInt32)(expected64 & 0xFFFFFFFFull);
      UInt32 expectedHigh = (UInt32)(expected64 >> 32);
      UInt32 desiredLow = (UInt32)((UInt64)desired & 0xFFFFFFFFull);
      UInt32 desiredHigh = (UInt32)((UInt64)desired >> 32);

      UInt8 success;

      asm volatile(
        "lock\ncmpxchg8b %1\n"
        "sete %0"
        : "=q"(success), "+m"(*(volatile UInt64*)RawPtr()),
          "+a"(expectedLow), "+d"(expectedHigh)
        : "b"(desiredLow), "c"(desiredHigh)
        : "cc", "memory"
      );

      expected = (ValueType)(((UInt64)expectedHigh << 32) | expectedLow);

      return success != 0;
    } else {
      static_assert(
        sizeof(ValueType) == 4 || sizeof(ValueType) == 8,
        "CAS: implement 1/2-byte if needed"
      );
    }
  }

  template <typename ValueType>
  ValueType IA32Atomic<ValueType>::DoFetchAdd(
    ValueType delta,
    AtomicMemoryOrder order
  ) {
    (void)order;

    if constexpr (sizeof(ValueType) == 4) {
      UInt32 val = (UInt32)delta;

      asm volatile(
        "lock\nxaddl %0, %1"
        : "+r"(val), "+m"(*(volatile UInt32*)RawPtr())
        :
        : "cc", "memory"
      );

      return (ValueType)val;
    } else {
      static_assert(
        sizeof(ValueType) == 4,
        "fetchAdd: implement other widths as needed"
      );
    }
  }

  template <typename ValueType>
  bool IA32Atomic<ValueType>::DoIsLockFree() const {
    if constexpr (
      sizeof(ValueType) == 1 ||
      sizeof(ValueType) == 2 ||
      sizeof(ValueType) == 4
    ) {
      return true;
    } else if constexpr (sizeof(ValueType) == 8) {
      return true;
    } else {
      return false;
    }
  }

  template class IA32Atomic<UInt32>;
}
