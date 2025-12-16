/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Exceptions.hpp
 * IA32 exception handler registration.
 */

#pragma once

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
  };
}
