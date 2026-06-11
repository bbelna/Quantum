/**
 * @file Include/Quantum/Clients/PCIDriverClient.hpp
 * @brief Declares @ref @QClients::PCIDriverClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/PCI.hpp>

#include "DriverClient.hpp"

namespace Quantum::Clients {
  /**
   * @brief Client-side interface for PCI bus driver operations.
   *
   * Extends @ref DriverClient with typed wrapper methods for all PCI
   * driver operations. The caller does not need to know whether the
   * driver is a kernel-mode or userspace driver.
   *
   * @code
   *   PCIDriverClient pci;
   *   pci.Initialize(pciDeviceID);
   *
   *   HAL::PCI::PCIDeviceInfo ide;
   *   if (pci.FindDeviceByClass(0x01, 0x01, ide)) {
   *     UInt32 bar4 = pci.ReadBAR(ide.Bus, ide.Slot, ide.Function, 4);
   *     pci.EnableBusMastering(ide.Bus, ide.Slot, ide.Function);
   *   }
   * @endcode
   */
  class PCIDriverClient : public DriverClient {
    public:

      // ----- Configuration space access -----

      /**
       * @brief Reads a 32-bit value from PCI configuration space.
       * @param bus PCI bus number.
       * @param slot Device slot number.
       * @param function Function number.
       * @param offset Dword-aligned register offset.
       * @return The 32-bit configuration register value.
       */
      UInt32 ReadConfig(
        UInt8 bus,
        UInt8 slot,
        UInt8 function,
        UInt8 offset
      );

      /**
       * @brief Writes a 32-bit value to PCI configuration space.
       * @param bus PCI bus number.
       * @param slot Device slot number.
       * @param function Function number.
       * @param offset Dword-aligned register offset.
       * @param value The 32-bit value to write.
       */
      void WriteConfig(
        UInt8 bus,
        UInt8 slot,
        UInt8 function,
        UInt8 offset,
        UInt32 value
      );

      // ----- Device discovery -----

      /**
       * @brief Finds a PCI device by vendor and device ID.
       * @param vendorID The PCI vendor ID to search for.
       * @param deviceID The PCI device ID to search for.
       * @param result [out] Receives the device information if found.
       * @return `true` if a matching device was found.
       */
      bool FindDeviceByID(
        UInt16 vendorID,
        UInt16 deviceID,
        HAL::PCI::PCIDeviceInfo& result
      );

      /**
       * @brief Finds a PCI device by class and subclass code.
       * @param classCode The PCI class code (e.g. `0x01` for mass storage).
       * @param subclassCode The PCI subclass code (e.g. `0x01` for IDE).
       * @param result [out] Receives the device information if found.
       * @return `true` if a matching device was found.
       */
      bool FindDeviceByClass(
        UInt8 classCode,
        UInt8 subclassCode,
        HAL::PCI::PCIDeviceInfo& result
      );

      // ----- BAR access -----

      /**
       * @brief Reads a Base Address Register (BAR) from a PCI device.
       * @param bus PCI bus number.
       * @param slot Device slot number.
       * @param function Function number.
       * @param barIndex BAR index (0-5).
       * @return The raw 32-bit BAR value.
       */
      UInt32 ReadBAR(
        UInt8 bus,
        UInt8 slot,
        UInt8 function,
        UInt8 barIndex
      );

      // ----- Bus mastering -----

      /**
       * @brief Enables PCI bus mastering for a device.
       *
       * Sets bit 2 (Bus Master Enable) in the PCI Command register,
       * allowing the device to initiate DMA transfers on the PCI bus.
       *
       * @param bus PCI bus number.
       * @param slot Device slot number.
       * @param function Function number.
       */
      void EnableBusMastering(
        UInt8 bus,
        UInt8 slot,
        UInt8 function
      );
  };
}
