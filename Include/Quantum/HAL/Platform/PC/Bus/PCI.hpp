/**
 * @file Include/Quantum/HAL/Platform/PC/Bus/PCI.hpp
 * @brief PCI bus type constant and address helpers for Device.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Device.hpp>

namespace Quantum::HAL {
  namespace DeviceBus {
    /**
     * @brief PCI (Peripheral Component Interconnect) bus.
     */
    constexpr UInt8 PCI = 3;
  }

  /**
   * @brief PCI bus address identifying a specific function on the bus.
   */
  struct PCIAddress {
    /**
     * @brief PCI bus number.
     */
    UInt8 Bus;

    /**
     * @brief Device slot number on the bus.
     */
    UInt8 Slot;

    /**
     * @brief Function number within the device.
     */
    UInt8 Function;
  };

  /**
   * @brief Stores a PCI address into a device's opaque bus data buffer.
   * @param device The device to update.
   * @param address The PCI address to store.
   */
  inline void SetPCIAddress(Device& device, const PCIAddress& address) {
    device.Bus = DeviceBus::PCI;
    device.BusDataSize = sizeof(PCIAddress);

    auto* dest = reinterpret_cast<PCIAddress*>(device.BusData);

    *dest = address;
  }

  /**
   * @brief Reads a PCI address from a device's opaque bus data buffer.
   * @param device The device to read from.
   * @return The PCI address stored in the device.
   */
  inline PCIAddress GetPCIAddress(const Device& device) {
    return *reinterpret_cast<const PCIAddress*>(device.BusData);
  }
}
