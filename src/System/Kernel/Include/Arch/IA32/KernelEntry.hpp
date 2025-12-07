//------------------------------------------------------------------------------
// Quantum
//------------------------------------------------------------------------------
// System/Kernel/Include/Arch/IA32/KernelEntry.hpp
// Declarations for the IA32 entry routines.
// Brandon Belna - MIT License
//------------------------------------------------------------------------------

#pragma once

extern "C" {
  /*
   * Sets up segment registers, stack, and calls into Kernel::Initialize().
   */
  void StartKernel();
}
