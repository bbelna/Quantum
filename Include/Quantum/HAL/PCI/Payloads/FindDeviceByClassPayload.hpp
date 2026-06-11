/**
 * @file Include/Quantum/HAL/PCI/Payloads/FindDeviceByClassPayload.hpp
 * @brief Declares @ref Quantum::HAL::PCI::Payloads::FindDeviceByClassPayload.
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
   * @brief In-out payload for @ref PCIDriverOperation::FindDeviceByClass.
   *
   * The caller fills in @ref ClassCode and @ref SubclassCode; the driver
   * fills in @ref Result if a matching device is found.
   */
  struct FindDeviceByClassPayload {
    /**
     * @brief [in] PCI class code to search for (e.g. `0x01` for mass
     *        storage).
     */
    UInt8 ClassCode;

    /**
     * @brief [in] PCI subclass code to search for (e.g. `0x01` for IDE
     *        controller).
     */
    UInt8 SubclassCode;

    /**
     * @brief [out] Information about the discovered device.
     */
    PCIDeviceInfo Result;
  };
}
