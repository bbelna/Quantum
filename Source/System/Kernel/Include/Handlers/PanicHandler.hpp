/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Handlers/PanicHandler.hpp
 * Kernel panic handler.
 */

#pragma once

#include <Types.hpp>
#include <String.hpp>

namespace Quantum::System::Kernel::Handlers {
  /**
   * Kernel panic handler.
   */
  class PanicHandler {
    public:
      /**
       * Handle a kernel panic.
       * @param message
       *   Panic message.
       * @param file
       *   The source file where the panic occurred (optional).
       * @param line
       *   The line number where the panic occurred (optional).
       * @param function
       *   The function name where the panic occurred (optional).
       */
      [[noreturn]]
      static void Handle(
        String message,
        String file = nullptr,
        UInt32 line = -1,
        String function = nullptr
      );
  };
}
