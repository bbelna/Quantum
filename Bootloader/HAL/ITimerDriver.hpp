/**
 * @file Bootloader/HAL/ITimerDriver.hpp
 * @brief Declares @ref @QBtldr::HAL::ITimerDriver.
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
   * @brief Abstract interface for timer drivers in the bootloader
   *        environment.
   */
  class ITimerDriver {
    public:
      /**
       * @brief Destroys this @ref ITimerDriver instance.
       */
      virtual ~ITimerDriver() = default;

      /**
       * @brief Returns the current time as a millisecond timestamp.
       * @return Milliseconds elapsed since an arbitrary platform-defined
       *         epoch (e.g. midnight for BIOS-based implementations).
       *
       * The absolute value is not meaningful; use the difference between
       * two calls to measure elapsed time.
       */
      virtual UInt32 GetMS() = 0;

      /**
       * @brief Blocks until at least @p ms milliseconds have elapsed.
       * @param ms Number of milliseconds to wait.
       *
       * The actual wait may be slightly longer than @p ms due to the
       * granularity of the underlying timer.
       */
      virtual void WaitMS(UInt32 ms) = 0;
  };
}
