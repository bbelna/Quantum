//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/Exceptions.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Default IA32 exception handler registration.
//------------------------------------------------------------------------------

#pragma once

namespace Quantum::Kernel::Arch::IA32 {
  /**
   * Installs default exception handlers for critical CPU faults.
   * Currently handles #DE (0), #GP (13), and #PF (14).
   */
  void InstallDefaultExceptionHandlers();
}
