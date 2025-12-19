/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/Task.hpp
 * User-mode task helpers.
 */

#pragma once

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

namespace Quantum {
  /**
   * User-mode task helpers.
   */
  class Task {
    public:
      /**
       * Yields the current task.
       */
      static inline void Yield() {
        ABI::InvokeSystemCall(ABI::SystemCall::Yield);
      }

      /**
       * Exits the current task.
       * @param code
       *   Optional exit code (currently ignored by the kernel).
       */
      static inline void Exit(UInt32 code = 0) {
        ABI::InvokeSystemCall(ABI::SystemCall::Exit, code);
      }
  };
}
