/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Exceptions.hpp
 * IA32 exception handler registration.
 */

#pragma once

#include <Types.hpp>

#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 exception handler registration.
   */
  class Exceptions {
    public:
      /**
       * Installs default exception handlers for critical CPU faults.
       * Currently handles #DE (0), #GP (13), and #PF (14).
       */
      static void InstallDefaultHandlers();

    private:
      /**
       * Dumps the interrupt context to the kernel log.
       * @param context
       *   The interrupt context.
       * @param faultAddress
       *   Optional, defaults to 0. The faulting address (for page faults).
       */
      static void DumpContext(
        const Interrupts::Context& context,
        UInt32 faultAddress = 0
      );

      /**
       * Default divide-by-zero fault handler.
       */
      static Interrupts::Context* OnDivideByZero(
        Interrupts::Context& context
      );

      /**
       * Default general protection fault handler.
       */
      static Interrupts::Context* OnGeneralProtection(
        Interrupts::Context& context
      );

      /**
       * Default page fault handler.
       */
      static Interrupts::Context* OnPageFault(
        Interrupts::Context& context
      );
  };
}
