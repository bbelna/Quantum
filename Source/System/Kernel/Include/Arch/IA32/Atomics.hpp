/**
 * @file System/Kernel/Include/Arch/IA32/Atomics.hpp
 * @brief IA32 atomic operations.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * Low-level atomic operations for IA32.
   */
  class Atomics {
    public:
      /**
       * Compiler-only memory barrier.
       */
      static inline void CompilerFence() {
        asm volatile("" ::: "memory");
      }

      /**
       * Full memory fence.
       */
      static inline void FullFence() {
        UInt32 scratch = 0;

        // use a locked instruction as a full fence on IA32
        asm volatile(
          "lock; addl $0, %0"
          : "+m"(scratch)
          :
          : "memory"
        );
      }

      /**
       * Atomically loads a 32-bit value.
       * @param address
       *   Address of the value to load.
       * @return
       *   Loaded value.
       */
      static inline UInt32 Load(const volatile UInt32* address) {
        UInt32 value = *address;

        CompilerFence();

        return value;
      }

      /**
       * Atomically stores a 32-bit value.
       * @param address
       *   Address of the value to store.
       * @param value
       *   Value to store.
       */
      static inline void Store(volatile UInt32* address, UInt32 value) {
        CompilerFence();

        *address = value;

        CompilerFence();
      }

      /**
       * Atomically exchanges a 32-bit value.
       * @param address
       *   Address of the value to exchange.
       * @param value
       *   Value to store.
       * @return
       *   Previous value.
       */
      static inline UInt32 Exchange(volatile UInt32* address, UInt32 value) {
        asm volatile(
          "xchgl %0, %1"
          : "+r"(value), "+m"(*address)
          :
          : "memory"
        );

        return value;
      }

      /**
       * Atomically compares and swaps a 32-bit value.
       * @param address
       *   Address of the value to compare/exchange.
       * @param expected
       *   Expected value; updated with the actual value on failure.
       * @param desired
       *   Value to store if expected matches.
       * @return
       *   True if the exchange succeeded.
       */
      static inline bool CompareExchange(
        volatile UInt32* address,
        UInt32& expected,
        UInt32 desired
      ) {
        UInt32 previous;
        UInt32 expectedValue = expected;

        asm volatile(
          "lock; cmpxchgl %2, %1"
          : "=a"(previous), "+m"(*address)
          : "r"(desired), "0"(expected)
          : "memory"
        );

        expected = previous;

        return previous == expectedValue;
      }

      /**
       * Atomically adds a delta and returns the previous value.
       * @param address
       *   Address of the value to add to.
       * @param delta
       *   Value to add.
       * @return
       *   Previous value.
       */
      static inline UInt32 FetchAdd(volatile UInt32* address, UInt32 delta) {
        asm volatile(
          "lock; xaddl %0, %1"
          : "+r"(delta), "+m"(*address)
          :
          : "memory"
        );

        return delta;
      }
  };
}
