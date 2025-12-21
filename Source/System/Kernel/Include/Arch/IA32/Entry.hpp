/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Entry.hpp
 * IA32 kernel entry routines.
 */

#pragma once

#include <Types.hpp>

extern "C" {
  /**
   * Sets up segment registers, stack, and calls into `Start()`.
   */
  void Entry();

  /**
   * The main kernel start routine called from `Entry()`.
   * @param bootInfoPhysicalAddress
   *   Physical address of the boot info block.
   */
  void Start(UInt32 bootInfoPhysicalAddress);
}

/**
 * Clears the BSS segment.
 */
void ClearBSS();

/**
 * Initializes logging.
 */
void InitializeLogging();

/**
 * Relocates the init bundle to a safe memory location.
 * @param bootInfoPhysicalAddress
 *   Physical address of the boot info block.
 */
void RelocateInitBundle(UInt32 bootInfoPhysicalAddress);