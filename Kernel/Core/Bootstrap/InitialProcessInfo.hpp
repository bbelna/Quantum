/**
 * @file Kernel/Bootstrap/InitialProcessInfo.hpp
 * @brief Declares @ref @QKrnl::InitialProcessInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Bootstrap {
  /**
   * @brief Information about the initial process passed from the bootloader.
   */
  struct InitialProcessInfo {
    /**
     * @brief Memory block containing the initial image (process binary +
     *        image data).
     */
    MemoryBlock ImageBlock;

    /**
     * @brief Offset of the process entry point from the image base.
     */
    UInt32 EntryPointOffset;

    /**
     * @brief Size of the process binary in bytes. Everything after this
     *        offset within the image block is image data passed to the
     *        process (e.g., a bundle of additional binaries).
     */
    UInt32 ProcessImageSize;

    /**
     * @brief Name of the initial process (e.g. `"StartupServer.qbn"`).
     */
    const char* Name;
  };
}
