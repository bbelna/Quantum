/**
 * @file Include/Quantum/HAL/PCI/PCIDeviceInfo.hpp
 * @brief Declares @ref Quantum::HAL::PCI::PCIDeviceInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::PCI {
  /**
   * @brief Information about a discovered PCI device, shared between the
   *        kernel PCI driver and userspace clients.
   */
  struct PCIDeviceInfo {
    /**
     * @brief PCI bus number (0-255).
     */
    UInt8 Bus;

    /**
     * @brief Device slot number (0-31).
     */
    UInt8 Slot;

    /**
     * @brief Function number (0-7).
     */
    UInt8 Function;

    /**
     * @brief PCI vendor ID.
     */
    UInt16 VendorID;

    /**
     * @brief PCI device ID.
     */
    UInt16 DeviceID;

    /**
     * @brief PCI class code (upper 8 bits of offset 0x08 >> 16).
     */
    UInt8 ClassCode;

    /**
     * @brief PCI subclass code (lower 8 bits of offset 0x08 >> 16).
     */
    UInt8 SubclassCode;

    /**
     * @brief Raw BAR0-5 values.
     *
     * For memory BARs, the lower 4 bits are flags and the upper 28 bits
     * are the base address (mask with `0xFFFFFFF0`). For I/O BARs, the
     * lower 2 bits are flags and the upper 30 bits are the base address
     * (mask with `0xFFFFFFFC`).
     */
    UInt32 BAR[6];
  };
}
