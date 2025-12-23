/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Floppy.hpp
 * IA32 floppy controller interrupt handling.
 */

#pragma once

#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 floppy controller interrupt handler.
   */
  class Floppy {
    public:
      /**
       * Initializes the floppy IRQ handler.
       */
      static void Initialize();

    private:
      /**
       * IRQ6 handler for floppy controller interrupts.
       * @param context
       *   Interrupt context.
       * @return
       *   Updated interrupt context.
       */
      static Interrupts::Context* IRQHandler(
        Interrupts::Context& context
      );
  };
}
