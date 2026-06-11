/**
 * @file Include/Quantum/HAL/IDriver.hpp
 * @brief Declaration of QuantumOS device driver interfaces and utilities.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "Device.hpp"

namespace Quantum::HAL {
  /**
   * @brief Abstract device driver interface.
   */
  class IDriver {
    public:
      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      virtual Device& GetDevice() = 0;

      /**
       * @brief Gets the unique identifier of the device associated with this
       *        driver.
       * @return The unique identifier of the device associated with this
       *         driver.
       */
      virtual DeviceID GetDeviceID() = 0;

      /**
       * @brief Invokes a driver-specific operation.
       * @param operation The driver-specific operation code.
       * @param payload Pointer to an optional operation-specific payload.
       * @return A driver-specific result value.
       */
      virtual UInt32 Invoke(UInt32 operation, void* payload) = 0;
  };
}
