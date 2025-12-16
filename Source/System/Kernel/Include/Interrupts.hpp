/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Interrupts.hpp
 * Architecture-agnostic interrupt controller for registering handlers.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel {
  struct InterruptContext;

  using InterruptHandler = void(*)(InterruptContext& ctx);

  /**
   * Kernel interrupt controller for registering handlers.
   */
  class Interrupts {
    public:
      /**
       * Initializes the kernel interrupt subsystem.
       */
      static void Initialize();

      /**
       * Registers an interrupt handler for the given vector.
       * @param vector
       *  The interrupt vector number.
       * @param handler
       *   The interrupt handler function.
       */
      static void RegisterHandler(UInt8 vector, InterruptHandler handler);
  };
}
