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
  };
}
