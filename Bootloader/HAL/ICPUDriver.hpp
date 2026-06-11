/**
 * @file Bootloader/HAL/ICPUDriver.hpp
 * @brief Declares @ref @QBtldr::HAL::ICPUDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <HAL/HALTypes.hpp>

namespace Quantum::Bootloader::HAL {
  /**
   * @brief Abstract interface for CPU-level operations in the bootloader
   *        environment.
   */
  class ICPUDriver {
    public:
      /**
       * @brief Destroys this @ref ICPUDriver instance.
       */
      virtual ~ICPUDriver() = default;

      /**
       * @brief Disables interrupts and halts the CPU indefinitely.
       *
       * Called on unrecoverable errors (missing file loader, failed
       * kernel load, etc.). Does not return.
       */
      [[noreturn]] virtual void HaltForever() = 0;
  };
}
