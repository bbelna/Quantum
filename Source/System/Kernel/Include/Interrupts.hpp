/**
 * @file System/Kernel/Include/Interrupts.hpp
 * @brief Architecture-agnostic interrupt handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#include "Arch/Interrupts.hpp"

namespace Quantum::System::Kernel {
  /**
   * Kernel interrupt controller for registering handlers.
   */
  class Interrupts {
    public:
      using Context = Arch::Interrupts::Context;

      /**
       * An interrupt handler function.
       * @param context
       *   Reference to the interrupt context.
       * @return
       *   Pointer to the context to resume after handling (can be the same
       *   context).
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
