/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapStage0.hpp
 * @brief Declares @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage0.
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
   * @brief Physical stack pointer used in @ref IA32BootstrapStage0.
   */
  constexpr UIntPtr IA32BootstrapStackPointer = 0x9F000;

  /**
   * @brief Stage 0 of the IA-32 kernel bootstrap process.
   *
   * This is the very first code executed by the kernel after being loaded
   * by the bootloader. It is a naked function containing inline assembly
   * placed in the `.text.start.entry` section so the linker positions it at
   * the kernel entry point.
   *
   * @ref IA32BootstrapStage0 performs the following steps, in order:
   *
   *   1. Disables interrupts (`cli`) to prevent any IRQs from firing before
   *      the IDT and interrupt controller are configured later in the boot.
   *
   *   2. Loads the 32-bit GDT (`lgdt`) so the CPU has valid segment
   *      descriptors for protected-mode operation.
   *
   *   3. Loads the kernel data segment selector (`0x10`) into all data segment
   *      registers (`DS`, `ES`, `SS`, `FS`, `GS`) so that subsequent memory
   *      accesses use the flat kernel data segment.
   *
   *   4. Sets `ESP` to @ref IA32BootstrapStackPointer, establishing the initial
   *      kernel stack in low memory below the video RAM region.
   *
   *   5. Pushes `ESI` onto the stack as the first argument to
   *      @ref IA32BootstrapStage1. `ESI` contains the physical address of the
   *      boot info structure, passed by the bootloader.
   *
   *   6. Calls @ref IA32BootstrapStage1, which continues the bootstrap process
   *      in C++.
   *
   *   7. If @ref IA32BootstrapStage1 ever returns (it should not), enters an 
   *      infinite halt loop (`hlt` / `jmp`) to prevent undefined execution.
   *
   * This function does not return.
   */
  extern "C" [[noreturn]] void IA32BootstrapStage0();
}
