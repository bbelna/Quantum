/**
 * @file Bootloader/Arch/x86/x86KernelLauncher.hpp
 * @brief Declares @ref @QBtldr::Arch::x86::x86KernelLauncher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Arch/x86/x86Types.hpp>

namespace Quantum::Bootloader::Arch::x86 {
  /**
   * @brief x86 implementation of @ref IKernelLauncher.
   *
   * Loads the physical address of the boot-info block into `ESI` and
   * performs an absolute indirect jump to the kernel entry point at physical
   * address `0x00100000`.
   */
  class x86KernelLauncher : public IKernelLauncher {
    public:
      /**
       * @brief Physical address of the kernel entry point.
       */
      static constexpr UInt32 KernelEntryPhysicalAddress = 0x00100000;

      /**
       * @brief Jumps to the kernel entry point. Does not return.
       * @param bootInfo Pointer to the boot-info block (physical address is
       *                 passed to the kernel in ESI).
       * @param context  Unused; present for interface conformance.
       */
      [[noreturn]] void Launch(
        IBootInfo* bootInfo,
        BootloaderContext* context
      ) override;
  };
}
