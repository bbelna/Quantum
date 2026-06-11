/**
 * @file Bootloader/IKernelLauncher.hpp
 * @brief Declares @ref @QBtldr::IKernelLauncher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <BootloaderTypes.hpp>
#include "BootloaderContext.hpp"
#include "IBootInfo.hpp"

namespace Quantum::Bootloader {
  /**
   * @brief Abstract interface for kernel launchers.
   */
  class IKernelLauncher {
    public:
      /**
       * @brief Launches the kernel with the given boot information.
       * @param bootInfo Pointer to the boot information structure to pass
       *                 to the kernel.
       */
      [[noreturn]] virtual void Launch(
        IBootInfo* bootInfo,
        BootloaderContext* context
      ) = 0;
  };
}
