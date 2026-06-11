/**
 * @file Include/Quantum/HAL/PCI/Payloads/FindDeviceByIDPayload.hpp
 * @brief Declares @ref Quantum::HAL::PCI::Payloads::FindDeviceByIDPayload.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "../PCIDeviceInfo.hpp"

namespace Quantum::HAL::PCI::Payloads {
  /**
   * @brief In-out payload for @ref PCIDriverOperation::FindDeviceByID.
   *
   * The caller fills in @ref VendorID and @ref DeviceID; the driver
   * fills in @ref Result if a matching device is found.
   */
  struct FindDeviceByIDPayload {
    /**
     * @brief [in] PCI vendor ID to search for.
     */
    UInt16 VendorID;

    /**
     * @brief [in] PCI device ID to search for.
     */
    UInt16 DeviceID;

    /**
     * @brief [out] Information about the discovered device.
     */
    PCIDeviceInfo Result;
  };
}
