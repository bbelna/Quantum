//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// Kernel/Include/Arch/X86/KernelEntry.hpp
// Declarations for the 32-bit entry routines.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

extern "C" {
  /*
   * Switches the CPU from real mode into 32-bit protected mode.
   */
  void EnterProtectedMode();

  /*
   * Sets up segment registers, stack, and calls into Kernel::Initialize().
   */
  void StartKernel();
}
