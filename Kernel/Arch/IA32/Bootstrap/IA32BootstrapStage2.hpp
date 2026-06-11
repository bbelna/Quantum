/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapStage2.hpp
 * @brief Declares @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage2.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include <Arch/IA32/Memory/E820BootInfo.hpp>
#include <Arch/IA32/Memory/IA32MemoryTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Bootstrap {
  /**
   * @brief Base address where the initial image is relocated during
   *        @ref IA32BootstrapStage2.
   *
   * The initial image is initially loaded by the bootloader at a low address
   * (e.g. `0x100000`), but the kernel executes in the higher-half
   * (e.g. `0xFFFFFFFF80000000`).
   *
   * To bridge this gap, @ref IA32BootstrapStage2 copies the initial image from
   * its initial location to this predefined higher-half address, allowing the
   * kernel to run from its expected address space with correct memory mappings.
   */
  constexpr UIntPtr IA32InitialImageRelocationBase = 0x00200000;

  /**
   * @brief E820 boot information populated by @ref IA32BootstrapStage2.
   * @see E820BootInfo
   *
   * This variable is set during @ref IA32BootstrapStage2 of the bootstrap process based
   * on the physical address passed in by @ref IA32BootstrapStage1.
   * 
   * It is used to initialize memory management and other subsystems that
   * require knowledge of the system's memory layout and available resources.
   */
  [[gnu::section(".start.data")]]
  inline E820BootInfo* BootInfo = nullptr;

  /**
   * @brief Stage 2 of the IA-32 kernel bootstrap process.
   * @param bootInfoPhysicalAddress Physical address for @ref E820BootInfo.
   *
   * Called by @ref IA32BootstrapStage1 after paging has been enabled. This is the first
   * stage that runs entirely in the higher-half of the kernel's address space.
   *
   * @ref IA32BootstrapStage2 performs the following steps, in order:
   *
   *   1. Relocates the initial image from its initial load address to
   *      @ref IA32InitialImageRelocationBase, updating the boot info
   *      structure to reflect the new location.
   *
   *   2. Runs all global constructors from the `.init_array` section so that
   *      static objects are properly initialized before kernel code uses them.
   *
   *   3. Transfers control to the platform-specific Stage 3, which
   *      completes kernel initialization.
   *
   * This function does not return.
   */
  extern "C"
  [[noreturn]]
  void IA32BootstrapStage2(UInt32 bootInfoPhysicalAddress);
}
