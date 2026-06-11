/**
 * @file Bootloader/IPlatformInitializer.hpp
 * @brief Declares @ref @QBtldr::IPlatformInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <BootloaderTypes.hpp>
#include "PlatformInitializationOptions.hpp"

namespace Quantum::Bootloader {
  /**
   * @brief Abstract interface for platform initializers.
   *
   * Concrete implementations are responsible for two distinct phases:
   *
   * 1. **Early init** via @ref Initialize, installs drivers into the
   *    @ref BootloaderContext and prepares the file system.
   * 2. **Kernel handoff** via @ref PrepareKernelHandoff, collects the E820
   *    memory map, sets up the VESA display mode, and writes initial image
   *    and process metadata into the boot-info block immediately before the
   *    kernel jump.
   */
  class IPlatformInitializer
    : public IInitializer<PlatformInitializationOptions, void> {
    public:
      /**
       * @brief Finalizes the boot-info block for kernel handoff.
       * @param options The same context and boot-info used during
       *                @ref Initialize.
       */
      virtual void PrepareKernelHandoff(
        PlatformInitializationOptions options
      ) = 0;
  };
}
