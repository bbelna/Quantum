/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/IRQ.hpp
 * Kernel IRQ routing subsystem.
 */

#pragma once

#include "Interrupts.hpp"
#include "Types.hpp"

namespace Quantum::System::Kernel {
  /**
   * Kernel IRQ routing subsystem.
   */
  class IRQ {
    public:
      /**
       * Registers a routing port for the given IRQ line.
       */
      static bool Register(UInt32 irq, UInt32 portId);

      /**
       * Unregisters a routing port for the given IRQ line.
       */
      static bool Unregister(UInt32 irq);

      /**
       * Enables an IRQ line.
       */
      static bool Enable(UInt32 irq);

      /**
       * Disables an IRQ line.
       */
      static bool Disable(UInt32 irq);

      /**
       * IRQ handler invoked by the IDT dispatcher.
       */
      static Interrupts::Context* HandleIRQ(Interrupts::Context& context);

    private:
      /**
       * Maximum number of IRQ lines supported.
       */
      static constexpr UInt32 _maxIRQs = 16;

      /**
       * Registered routing port per IRQ line.
       */
      inline static UInt32 _irqPorts[_maxIRQs] = {};

      /**
       * Routes a specific IRQ to its registered port.
       */
      static void Notify(UInt32 irq);
  };
}
