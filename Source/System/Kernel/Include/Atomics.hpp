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

      constexpr Atomic() : _value(0) {}

      explicit constexpr Atomic(T value)
        : _value(static_cast<UInt32>(value)) {}

      T Load() const {
        return static_cast<T>(Arch::Atomics::Load(&_value));
      }

      void Store(T value) {
        Arch::Atomics::Store(&_value, static_cast<UInt32>(value));
      }

      T Exchange(T value) {
        return static_cast<T>(
          Arch::Atomics::Exchange(&_value, static_cast<UInt32>(value))
        );
      }

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

      T FetchAdd(T delta) {
        return static_cast<T>(
          Arch::Atomics::FetchAdd(&_value, static_cast<UInt32>(delta))
        );
      }

    private:
      mutable volatile UInt32 _value;
  };
}
