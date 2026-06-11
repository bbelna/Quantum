/**
 * @file Kernel/Arch/IA32/Bootstrap/IA32BootstrapStage0.cpp
 * @brief Implements @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage0.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "IA32BootstrapStage0.hpp"
#include "IA32BootstrapStage1.hpp"

namespace Quantum::Kernel::Arch::IA32::Bootstrap {
  extern "C"
  [[gnu::section(".text.start.entry")]]
  [[gnu::naked]]
  void IA32BootstrapStage0() {
    asm volatile(
      "cli\n"                      // clear interrupts
      "lgdt [IA32GDTDescriptor]\n" // load GDT for protected mode
      "mov $0x10, %%ax\n"          // data segment selector
      "mov %%ax, %%ds\n"           // <-|
      "mov %%ax, %%es\n"           //   |
      "mov %%ax, %%ss\n"           //   | set data segment registers
      "mov %%ax, %%fs\n"           //   |
      "mov %%ax, %%gs\n"           // <-|
      "mov $%c0, %%esp\n"          // stack above startup end, below video RAM
      "push %%esi\n"               // push boot info physical address
      "call IA32BootstrapStage1\n" // jump to C++ stage 1 bootstrap
      "add $4, %%esp\n"            // clean up stack after call
      "1:\n"                       // <-|
      "hlt\n"                      //   | halt loop if returns (it shouldn't)
      "jmp 1b\n"                   // <-|
      :
      : "i"(IA32BootstrapStackPointer)
      : "ax", "dx", "cc", "memory"
    );
  }
}
