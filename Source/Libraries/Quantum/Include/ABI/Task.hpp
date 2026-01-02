/**
 * @file Libraries/Quantum/Include/ABI/Task.hpp
 * @brief User-mode task library.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * User-mode task helpers.
   */
  class Task {
    public:
      /**
       * Yields the current task.
       */
      static inline void Yield() {
        ABI::InvokeSystemCall(ABI::SystemCall::Task_Yield);
      }

      /**
       * Exits the current task.
       * @param code
       *   Optional exit code (currently ignored by the kernel).
       */
      static inline void Exit(UInt32 code = 0) {
        ABI::InvokeSystemCall(ABI::SystemCall::Task_Exit, code);
      }

      /**
       * Sleeps for at least the specified number of timer ticks.
       * @param ticks
       *   Number of timer ticks to sleep.
       */
      static inline void SleepTicks(UInt32 ticks) {
        ABI::InvokeSystemCall(ABI::SystemCall::Task_Sleep, ticks);
      }

      /**
       * Returns the kernel tick rate in Hz.
       * @return
       *   Timer tick rate in Hz, or 0 on failure.
       */
      static inline UInt32 GetTickRate() {
        if (_cachedTickRate != 0) {
          return _cachedTickRate;
        }

        UInt32 hz = ABI::InvokeSystemCall(ABI::SystemCall::Task_GetTickRate);

        if (hz != 0) {
          _cachedTickRate = hz;
        }

        return hz;
      }

      /**
       * Sleeps for at least the specified number of milliseconds.
       * @param ms
       *   Milliseconds to sleep.
       */
      static inline void SleepMs(UInt32 ms) {
        UInt32 hz = GetTickRate();

        if (hz == 0) {
          return;
        }

        UInt64 ticks = static_cast<UInt64>(ms) * hz;
        ticks = (ticks + 999) / 1000;

        if (ticks == 0) {
          ticks = 1;
        }

        SleepTicks(static_cast<UInt32>(ticks));
      }

      /**
       * Sleeps for at least the specified number of microseconds.
       * @param us
       *   Microseconds to sleep.
       */
      static inline void SleepUs(UInt32 us) {
        UInt32 hz = GetTickRate();

        if (hz == 0) {
          return;
        }

        UInt64 ticks = static_cast<UInt64>(us) * hz;
        ticks = (ticks + 999999) / 1000000;

        if (ticks == 0) {
          ticks = 1;
        }

        SleepTicks(static_cast<UInt32>(ticks));
      }

    private:
      /**
       * Cached tick rate in Hz (0 = unknown).
       */
      inline static UInt32 _cachedTickRate = 0;
  };
}
