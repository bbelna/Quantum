/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/KernelEntry.hpp
 * IA32 kernel entry routines.
 */

#pragma once

#include <Types/Primitives.hpp>

extern "C" {
  /**
   * Sets up segment registers, stack, and calls into `StartKernel()`.
   */
  void KernelEntry();

  /**
   * The main kernel start routine called from `KernelEntry()`.
   * @param bootInfoPhysicalAddress
   *   Physical address of the boot info block.
   */
  void StartKernel(UInt32 bootInfoPhysicalAddress);
}

/**
 * Clears the BSS segment.
 */
void ClearBSS();

/**
 * Initializes kernel logging.
 */
void InitializeKernelLogging();

/**
 * Traces version and copyright information to the console.
 */
void TraceVersionAndCopyright();
