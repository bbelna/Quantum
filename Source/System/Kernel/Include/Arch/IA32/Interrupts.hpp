/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Interrupts.hpp
 * IA32 interrupt controller for registering handlers.
 */

#pragma once

#include <Prelude.hpp>
#include <Types/Primitives.hpp>
#include <Types/Interrupts/InterruptHandler.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using Kernel::Types::Interrupts::InterruptHandler;

  /**
   * IA32 kernel interrupt controller for registering handlers.
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
