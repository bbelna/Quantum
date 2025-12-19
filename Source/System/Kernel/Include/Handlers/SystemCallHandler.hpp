/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Handlers/SystemCallHandler.hpp
 * System call handler.
 */

#pragma once

#include <Interrupts.hpp>

namespace Quantum::System::Kernel::Handlers {
  /**
   * System call handler.
   */
  class SystemCallHandler {
    public:
      /**
       * Handles a system call.
       * @param context
       *   Interrupt context at the time of the system call.
       * @return
       *   Updated interrupt context.
       */
      static Interrupts::Context* Handle(Interrupts::Context& context);
  };
}
