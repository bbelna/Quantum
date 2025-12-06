//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Include/Arch/x86/KernelEntry.hpp
// Declarations for the 32-bit entry routines.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

extern "C" {
  /*
   * Sets up segment registers, stack, and calls into Kernel::Initialize().
   */
  void StartKernel();
}
