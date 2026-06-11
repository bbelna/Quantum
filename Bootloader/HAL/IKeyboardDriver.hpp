/**
 * @file Bootloader/HAL/IKeyboardDriver.hpp
 * @brief Declares @ref @QBtldr::HAL::IKeyboardDriver.
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
   * @brief Abstract interface for keyboard drivers in the bootloader
   *        environment.
   */
  class IKeyboardDriver {
    public:
      /**
       * @brief Destroys this @ref IKeyboardDriver instance.
       */
      virtual ~IKeyboardDriver() = default;

      /**
       * @brief Checks whether a key is waiting in the input buffer without
       *        consuming it.
       * @return `true` if at least one key is available, `false` otherwise.
       *
       * Used for non-blocking polling (e.g. countdown timers) where the
       * caller needs to do other work while waiting for input.
       */
      virtual bool IsKeyReady() = 0;

      /**
       * @brief Reads one key from the keyboard (blocking).
       * @return The ASCII code of the key, or `0` for non-ASCII extended keys.
       *
       * Blocks until a key is available. For non-blocking use, check
       * @ref IsKeyReady first.
       */
      virtual UInt8 ReadKey() = 0;
  };
}
