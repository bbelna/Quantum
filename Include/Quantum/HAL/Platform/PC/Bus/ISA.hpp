/**
 * @file Include/Quantum/HAL/Platform/PC/Bus/ISA.hpp
 * @brief ISA bus type constant and address helpers for Device.
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
     * @brief ISA (Industry Standard Architecture) bus.
     */
    constexpr UInt8 ISA = 2;
  }

  /**
   * @brief ISA bus address identifying a device by its I/O port and IRQ.
   */
  struct ISAAddress {
    /**
     * @brief Base I/O port address.
     */
    UInt16 IOBase;

    /**
     * @brief IRQ line number.
     */
    UInt8 IRQ;
  };

  /**
   * @brief Stores an ISA address into a device's opaque bus data buffer.
   * @param device The device to update.
   * @param address The ISA address to store.
   */
  inline void SetISAAddress(Device& device, const ISAAddress& address) {
    device.Bus = DeviceBus::ISA;
    device.BusDataSize = sizeof(ISAAddress);

    auto* dest = reinterpret_cast<ISAAddress*>(device.BusData);
    *dest = address;
  }

  /**
   * @brief Reads an ISA address from a device's opaque bus data buffer.
   * @param device The device to read from.
   * @return The ISA address stored in the device.
   */
  inline ISAAddress GetISAAddress(const Device& device) {
    return *reinterpret_cast<const ISAAddress*>(device.BusData);
  }
}
