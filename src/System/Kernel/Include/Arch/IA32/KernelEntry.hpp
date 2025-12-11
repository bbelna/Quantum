//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/KernelEntry.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Declarations for the IA32 entry routines.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>

extern "C" {
  /*
   * Sets up segment registers, stack, and calls into `StartKernel()`.
   */
  void KernelEntry();

  /**
   * The main kernel start routine called from `KernelEntry()`.
   */
  void StartKernel(UInt32 bootInfoPhys);
}

/**
 * Clears the BSS segment.
 */
void ClearBSS();
