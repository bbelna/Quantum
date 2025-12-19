/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Interrupts.hpp
 * IA32 interrupt controller for registering handlers.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 kernel interrupt controller for registering handlers.
   */
  class Interrupts {
    public:
      /**
       * Register snapshot captured on interrupt entry for IA32.
       */
      struct Context {
        /**
         * General-purpose register EDI (pusha order).
         */
        UInt32 EDI;

        /**
         * General-purpose register ESI (pusha order).
         */
        UInt32 ESI;

        /**
         * Base pointer captured during pusha.
         */
        UInt32 EBP;

        /**
         * Value before pusha for ESP.
         */
        UInt32 ESP;

        /**
         * General-purpose register EBX (pusha order).
         */
        UInt32 EBX;

        /**
         * General-purpose register EDX (pusha order).
         */
        UInt32 EDX;

        /**
         * General-purpose register ECX (pusha order).
         */
        UInt32 ECX;

        /**
         * General-purpose register EAX (pusha order).
         */
        UInt32 EAX;

        /**
         * Software-pushed vector.
         */
        UInt32 Vector;

        /**
         * Software-pushed hardware/synthetic error code.
         */
        UInt32 ErrorCode;

        /**
         * Instruction pointer at the time of the interrupt.
         */
        UInt32 EIP;

        /**
         * Code segment selector at the time of the interrupt.
         */
        UInt32 CS;

        /**
         * CPU flags at the time of the interrupt.
         */
        UInt32 EFlags;
      };

      /**
       * An interrupt handler function for IA32.
       */
      using Handler = Context* (*)(Context& context);

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
      static void RegisterHandler(UInt8 vector, Handler handler);
  };
}
