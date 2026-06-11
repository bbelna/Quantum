/**
 * @file Kernel/Arch/IA32/Interrupts/IA32IDTExceptionHandler.cpp
 * @brief Defines and implements
 *        @ref @QKrnlIA32::Interrupts::IA32IDTExceptionHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Drivers/Interrupts/IInterruptControllerDriver.hpp>
#include <Interrupts/IInterruptManager.hpp>
#include <KernelContext.hpp>
#include <KernelLog.hpp>

#include "IA32IDT.hpp"
#include "IA32InterruptDispatcher.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief Exception handler called from ISR stubs.
   * @param context The interrupt context.
   * @return The next interrupt context to use.
   */
  extern "C"
  IA32InterruptContext* IA32IDTExceptionHandler(
    IA32InterruptContext* context
  ) {
    IInterruptManager<IA32InterruptVector>* manager
      = Context->InterruptManager;

    if (!manager) {
      // send EOI for IRQs so the PIC doesn't stall
      UInt8 vector = static_cast<UInt8>(context->Vector);

      if (
        vector >= IRQ_BASE_VECTOR &&
        vector < IRQ_BASE_VECTOR + IRQ_COUNT
      ) {
        if (vector >= IRQ_BASE_VECTOR + 8) {
          asm volatile(
            "outb %0, %1" :: "a"(static_cast<UInt8>(PIC_EOI)),
            "Nd"(static_cast<UInt16>(PIC2_COMMAND_PORT))
          );
        }

        asm volatile(
          "outb %0, %1" :: "a"(static_cast<UInt8>(PIC_EOI)),
          "Nd"(static_cast<UInt16>(PIC1_COMMAND_PORT))
        );
      }

      return context;
    } else {
      return static_cast<IA32InterruptContext*>(
        manager->HandleException(context)
      );
    }
  }
}
