/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapStage1.hpp
 * @brief Declares @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage1.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Bootstrap {
  /**
   * @brief Stage 1 of the IA-32 kernel bootstrap process.
   * @param bootInfoPhysicalAddress Physical address for @ref E820BootInfo.
   *
   * Called by @ref IA32BootstrapStage0 with the boot info physical address.
   * This stage runs from low addresses (placed in the `.text.start` section)
   * and is responsible for enabling paging so the kernel can execute from its
   * higher-half address space.
   *
   * @ref IA32BootstrapStage1 performs the following steps, in order:
   *
   *   1. Clears the initial page directory.
   *
   *   2. Enables PSE (Page Size Extensions) and identity maps the first
   *      3 GB of physical memory using 4 MB pages so that code continues
   *      to execute correctly after paging is enabled.
   *
   *   3. Maps the kernel image into the higher-half address space,
   *      creating page table entries that map each kernel page from its load
   *      address to its linked virtual address.
   *
   *   4. Installs a recursive mapping in the page directory so the kernel can
   *      later manipulate page tables.
   *
   *   5. Loads the page directory into `CR3` and enables paging.
   *
   *   6. Jumps to @ref IA32BootstrapStage2 at its higher-half address, preserving
   *      the current (low-memory) stack and passing the boot info physical
   *      address as the argument.
   *
   * This function does not return.
   */
  extern "C" [[noreturn]] void IA32BootstrapStage1(UInt32 bootInfoPhysicalAddress);
}
