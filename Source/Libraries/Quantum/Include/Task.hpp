/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/Task.hpp
 * User-mode task helpers.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/ABI/SystemCall.hpp>

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
        Types::ABI::Invoke(Types::ABI::SystemCallId::Yield);
      }

      /**
       * Exits the current task.
       * @param code
       *   Optional exit code (currently ignored by the kernel).
       */
      static inline void Exit(UInt32 code = 0) {
        Types::ABI::Invoke(Types::ABI::SystemCallId::Exit, code);
      }
  };
}
