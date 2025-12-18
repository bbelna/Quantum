/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/SystemCall.hpp
 * System call dispatcher interface.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/ABI/SystemCallId.hpp>
#include <Types/Interrupts/InterruptContext.hpp>

namespace Quantum::System::Kernel {
  using Types::Interrupts::InterruptContext;
  using ::Quantum::Types::ABI::SystemCallId;

  /**
   * System call dispatcher.
   */
  class SystemCall {
    public:
      /**
       * Handles a system call interrupt.
       * @param context
       *   Interrupt context for the call.
       * @return
       *   Context to resume execution with.
       */
      static InterruptContext* Handle(InterruptContext& context);
  };
}
