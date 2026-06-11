/**
 * @file Bootloader/Platform/PC/HAL/BIOSTimerDriver.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::HAL::BIOSTimerDriver.
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
   * @brief BIOS-based timer driver for the bootloader environment.
   *
   * Implements @ref ITimerDriver using the BIOS system timer via
   * `INT 1Ah` / `AH=00h`. The underlying counter increments at
   * approximately 18.2 Hz (~54.9 ms per tick), giving a timer
   * resolution of roughly 55 ms.
   */
  class BIOSTimerDriver : public BIOSDriver, public ITimerDriver {
    public:
      /**
       * @brief Returns the current time in milliseconds since midnight.
       * @return Milliseconds derived from the BIOS tick counter
       *         (`CX:DX * 10000 / 182`).
       *
       * Issues `INT 1Ah` / `AH=00h` to read the 32-bit tick count and
       * converts it to milliseconds. Use the difference between two calls
       * to measure elapsed time; do not rely on the absolute value.
       */
      UInt32 GetMS() override;

      /**
       * @brief Blocks until at least @p ms milliseconds have elapsed.
       * @param ms Number of milliseconds to wait. Rounded up to the next
       *           tick boundary (~55 ms) due to timer granularity.
       *
       * Polls @ref GetMS in a tight loop until the requested duration
       * has passed.
       */
      void WaitMS(UInt32 ms) override;
  };
}
