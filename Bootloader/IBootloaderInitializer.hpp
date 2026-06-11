/**
 * @file Bootloader/IBootloaderInitializer.hpp
 * @brief Declares @ref @QBtldr::IBootloaderInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <BootloaderTypes.hpp>

namespace Quantum::Bootloader {
  struct BootloaderContext;

  /**
   * @brief Abstract interface for bootloader initializers.
   */
  class IBootloaderInitializer
    : public IInitializer<BootloaderContext*, void> {};
}
