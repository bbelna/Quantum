/**
 * @file Bootloader/Platform/PC/HAL/BIOSKeyboardDriver.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::HAL::BIOSKeyboardDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "BIOSDriver.hpp"

namespace Quantum::Bootloader::Platform::PC::HAL {
  /**
   * @brief BIOS-based keyboard driver for the bootloader environment.
   *
   * Implements @ref IKeyboardDriver using BIOS `INT 16h` calls via the
   * real-mode trampoline inherited from @ref BIOSDriver.
   */
  class BIOSKeyboardDriver : public BIOSDriver, public IKeyboardDriver {
    public:
      /**
       * @brief Checks whether a key is waiting in the BIOS keyboard buffer
       *        without consuming it.
       * @return `true` if at least one key is available, `false` otherwise.
       *
       * Issues `INT 16h` / `AH=01h`. The BIOS sets `ZF=1` when the buffer
       * is empty; this method returns `true` when `ZF=0`.
       */
      bool IsKeyReady() override;

      /**
       * @brief Reads one key from the BIOS keyboard buffer (blocking).
       * @return The ASCII code of the pressed key, or `0` for non-ASCII
       *         extended keys (e.g. function keys, arrow keys).
       *
       * Issues `INT 16h` / `AH=00h`, which blocks until a key is available.
       * The scan code is in `AH` and the ASCII code in `AL` on return;
       * only `AL` is returned here.
       */
      UInt8 ReadKey() override;
  };
}
