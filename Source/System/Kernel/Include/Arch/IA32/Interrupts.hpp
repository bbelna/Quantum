/**
 * @file System/Kernel/Include/Arch/IA32/Interrupts.hpp
 * @brief IA32 interrupt handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

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
        UInt32 edi;

        /**
         * General-purpose register ESI (pusha order).
         */
        UInt32 esi;

        /**
         * Base pointer captured during pusha.
         */
        UInt32 ebp;

        /**
         * Value before pusha for ESP.
         */
        UInt32 esp;

        /**
         * General-purpose register EBX (pusha order).
         */
        UInt32 ebx;

        /**
         * General-purpose register EDX (pusha order).
         */
        UInt32 edx;

        /**
         * General-purpose register ECX (pusha order).
         */
        UInt32 ecx;

        /**
         * General-purpose register EAX (pusha order).
         */
        UInt32 eax;

        /**
         * Software-pushed vector.
         */
        UInt32 vector;

        /**
         * Software-pushed hardware/synthetic error code.
         */
        UInt32 errorCode;

        /**
         * Instruction pointer at the time of the interrupt.
         */
        UInt32 eip;

        /**
         * Code segment selector at the time of the interrupt.
         */
        UInt32 cs;

        /**
         * CPU flags at the time of the interrupt.
         */
        UInt32 eflags;
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

      /**
       * Sends an End Of Interrupt (EOI).
       * @param irq
       *   IRQ number (0-15) that just fired.
       */
      static void End(UInt8 irq);

      /**
       * Masks (disables) a specific IRQ line.
       * @param irq
       *   IRQ number (0-15) to mask.
       */
      static void Mask(UInt8 irq);

      /**
       * Masks all IRQ lines.
       */
      static void MaskAll();

      /**
       * Unmasks (enables) a specific IRQ line.
       * @param irq
       *   IRQ number (0-15) to unmask.
       */
      static void Unmask(UInt8 irq);

      /**
       * Unmasks all IRQ lines.
       */
      static void UnmaskAll();
  };
}
