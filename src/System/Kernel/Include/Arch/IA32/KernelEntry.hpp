//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/KernelEntry.hpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// Declarations for the IA32 entry routines.
//------------------------------------------------------------------------------

#pragma once

#include <KernelTypes.hpp>

extern "C" {
  /*
   * Sets up segment registers, stack, and calls into `StartKernel()`.
   */
  void KernelEntry();

  /**
   * The main kernel start routine called from `KernelEntry()`.
   */
  void StartKernel(uint32 bootInfoPhys);
}

/**
 * Clears the BSS segment.
 */
void ClearBSS();
