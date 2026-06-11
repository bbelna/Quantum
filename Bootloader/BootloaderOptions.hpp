/**
 * @file Bootloader/BootloaderOptions.hpp
 * @brief Declares @ref @QBtldr::BootloaderOptions.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Bootloader {
  /**
   * @brief Options structure containing bootloader options.
   */
  struct BootloaderOptions {
    /**
     * @brief Path to the kernel file.
     */
    const char* KernelPath;

    /**
     * @brief Path to the initial image file.
     */
    const char* InitialImagePath;

    /**
     * @brief Name of the initial process (e.g. `"StartupServer.qbn"`).
     */
    const char* InitialProcessName;
  };
}
