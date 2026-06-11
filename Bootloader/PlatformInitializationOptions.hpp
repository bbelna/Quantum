/**
 * @file Bootloader/PlatformInitializationOptions.hpp
 * @brief Declares @ref @QBtldr::PlatformInitializationOptions.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IBootInfo.hpp"

namespace Quantum::Bootloader {
  struct BootloaderContext;

  /**
   * @brief Options passed to @ref IPlatformInitializer for both early
   *        initialization and kernel-handoff finalization.
   *
   * Carries pointers to the shared @ref BootloaderContext and the
   * platform-specific boot-information block so that the platform
   * initializer can populate drivers and finalize boot-info fields
   * without taking a raw integer address.
   */
  struct PlatformInitializationOptions {
    /**
     * @brief Pointer to the bootloader context to populate.
     */
    BootloaderContext* Context;

    /**
     * @brief Pointer to the boot-information block.
     *
     * Concrete @ref IPlatformInitializer implementations may downcast
     * this to their architecture-specific @ref IBootInfo descendant.
     */
    IBootInfo* Info;
  };
}
