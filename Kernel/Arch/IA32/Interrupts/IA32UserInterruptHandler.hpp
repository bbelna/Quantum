/**
 * @file Kernel/Arch/IA32/Interrupts/IA32UserInterruptTypes.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::UserInterruptHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IA32InterruptResource.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief Handler for user-claimed hardware interrupts. Registered with
   *        the IDT for each claimed IRQ vector. Wakes the waiting thread or
   *        increments the pending count.
   */
  class IA32UserInterruptHandler {
    public:
      /**
       * @brief Initializes the handler with the kernel context.
       * @param context Pointer to the kernel context.
       */
      static void Initialize(KernelContext* context);

      /**
       * @brief Handles a user-claimed hardware interrupt.
       * @param context The interrupt context from the ISR stub.
       * @return Always `nullptr` (no context switch from IRQ handler).
       *
       * Looks up the `InterruptResource` for the fired vector. If a thread
       * is blocked in `Interrupt_Wait`, it is woken and made runnable.
       * Otherwise, the pending count is incremented so that the next
       * `Interrupt_Wait` returns immediately. Sends EOI to the interrupt
       * controller before returning.
       */
      static IInterruptContext* Handle(IInterruptContext& context);

    private:
      /**
       * @brief Pointer to the kernel context.
       */
      static KernelContext* _context;
  };
}
