/**
 * @file System/Kernel/Include/Atomics.hpp
 * @brief Architecture-agnostic atomics wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Arch/Atomics.hpp"

namespace Quantum::System::Kernel {
  /**
   * Compiler-only memory barrier.
   */
  inline void CompilerFence() {
    Arch::Atomics::CompilerFence();
  }

  /**
   * Full memory fence.
   */
  inline void FullFence() {
    Arch::Atomics::FullFence();
  }

  /**
   * 32-bit atomic wrapper.
   */
  template <typename T>
  class Atomic {
    public:
      static_assert(
        sizeof(T) == sizeof(UInt32),
        "Atomic<T> only supports 32-bit types"
      );

      /**
       * Constructs an atomic with an initial value of zero.
       */
      constexpr Atomic() : _value(0) {}

      /**
       * Constructs an atomic with the given initial value.
       * @param value
       *   Initial value.
       */
      explicit constexpr Atomic(T value)
        : _value(static_cast<UInt32>(value)) {}

      /**
       * Atomically loads the value.
       * @return
       *   Loaded value.
       */
      T Load() const {
        return static_cast<T>(Arch::Atomics::Load(&_value));
      }

      /**
       * Atomically stores the value.
       * @param value
       *   Value to store.
       */
      void Store(T value) {
        Arch::Atomics::Store(&_value, static_cast<UInt32>(value));
      }

      /**
       * Atomically exchanges the value.
       * @param value
       *   Value to store.
       * @return
       *   Previous value.
       */
      T Exchange(T value) {
        return static_cast<T>(
          Arch::Atomics::Exchange(&_value, static_cast<UInt32>(value))
        );
      }

      /**
       * Atomically compares and swaps the value.
       * @param expected
       *   Expected value; updated with the actual value on failure.
       * @param desired
       *   Value to store if expected matches.
       * @return
       *   True if the exchange succeeded.
       */
      bool CompareExchange(T& expected, T desired) {
        UInt32 expectedValue = static_cast<UInt32>(expected);
        bool ok = Arch::Atomics::CompareExchange(
          &_value,
          expectedValue,
          static_cast<UInt32>(desired)
        );

        expected = static_cast<T>(expectedValue);

        return ok;
      }

      /**
       * Atomically adds a delta and returns the previous value.
       * @param delta
       *   Value to add.
       * @return
       *   Previous value.
       */
      T FetchAdd(T delta) {
        return static_cast<T>(
          Arch::Atomics::FetchAdd(&_value, static_cast<UInt32>(delta))
        );
      }

    private:
      /**
       * Underlying atomic value.
       */
      mutable volatile UInt32 _value;
  };
}
