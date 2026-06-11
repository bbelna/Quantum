/**
 * @file Bootloader/Arch/x86/x86KernelLauncher.cpp
 * @brief Implements @ref @QBtldr::Arch::x86::x86KernelLauncher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "x86KernelLauncher.hpp"

namespace Quantum::Bootloader::Arch::x86 {
  [[noreturn]] void x86KernelLauncher::Launch(
    IBootInfo* bootInfo,
    BootloaderContext* context
  ) {
    (void)context;

    // the physical address of the boot-info block is the pointer value itself
    // (no-paging, flat 32-bit PM); the kernel expects it in ESI
    UInt32 bootInfoAddress = reinterpret_cast<UInt32>(bootInfo);

    asm volatile(
      "movl %0, %%esi\n"
      "jmp *%1\n"
      :
      : "r"(bootInfoAddress), "r"(KernelEntryPhysicalAddress)
      : "esi"
    );

    __builtin_unreachable();
  }
}
