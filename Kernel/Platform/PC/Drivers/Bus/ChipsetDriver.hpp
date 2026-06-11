/**
 * @file Kernel/Platform/PC/Drivers/Bus/ChipsetDriver.hpp
 * @brief Declares @ref @QKrnlPC::Drivers::Bus::ChipsetDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Platform/PC/Bus/PCI.hpp>

#include <Drivers/DriverTypes.hpp>

#include "PCI.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Bus {
  /**
   * @brief PCI chipset identification driver.
   *
   * Probes PCI bus 0 at construction time to identify the northbridge
   * (slot 0, function 0) and southbridge (typically slot 7 for Intel,
   * slot 1 for others). Vendor and device IDs are matched against a
   * table of known chipsets to produce a human-readable display name.
   */
  class ChipsetDriver : public IDriver {
    public:
      /**
       * @brief Constructs the chipset driver and probes PCI bus 0.
       * @param pci Pointer to the PCI configuration space driver.
       */
      ChipsetDriver(PCI* pci);

      /**
       * @brief Gets the device associated with this driver.
       * @return The device associated with this driver.
       */
      Device& GetDevice() override { return _device; }

      /**
       * @brief Gets the unique identifier of the device.
       * @return The device ID.
       */
      DeviceID GetDeviceID() override { return _device.ID; }

      /**
       * @brief Invokes a chipset driver operation.
       * @param operation The operation code (reserved for future use).
       * @param payload Pointer to an operation-specific payload.
       * @return Operation-specific result (currently always 0).
       */
      UInt32 Invoke(UInt32 operation, void* payload) override;

    private:
      /**
       * @brief Entry in the known chipset lookup table.
       */
      struct ChipsetEntry {
        /**
         * @brief PCI vendor ID.
         */
        UInt16 VendorID;

        /**
         * @brief PCI device ID.
         */
        UInt16 DeviceID;

        /**
         * @brief Human-readable chipset name.
         */
        const char* Name;
      };

      /**
       * @brief Known northbridge chipsets.
       */
      static constexpr ChipsetEntry NorthbridgeTable[] = {
        { 0x8086, 0x122D, "Intel 430FX" },
        { 0x8086, 0x1237, "Intel 440FX" },
        { 0x8086, 0x7190, "Intel 440BX" },
        { 0x8086, 0x7192, "Intel 440BX-ZX" },
        { 0x1106, 0x0691, "VIA Apollo Pro" },
      };

      /**
       * @brief Known southbridge chipsets.
       */
      static constexpr ChipsetEntry SouthbridgeTable[] = {
        { 0x8086, 0x122E, "Intel PIIX" },
        { 0x8086, 0x7000, "Intel PIIX3" },
        { 0x8086, 0x7110, "Intel PIIX4" },
        { 0x1106, 0x0586, "VIA VT82C586B" },
      };

      /**
       * @brief PCI class code for ISA bridge (`0x0601`).
       *
       * Used when scanning southbridge candidates to identify the ISA
       * bridge function.
       */
      static constexpr UInt16 PCIClassISABridge = 0x0601;

      /**
       * @brief Pointer to the PCI configuration space driver.
       */
      PCI* _pci = nullptr;

      /**
       * @brief The device associated with this driver.
       */
      Device _device {
        7,
        "Chipset",
        "Unknown Chipset",
        ToDeviceCategoryID(DeviceCategoryType::Chipset),
        DeviceState::Active,
        0,
        0,
        DeviceBus::PCI,
        0,
        {}
      };

      /**
       * @brief Probes PCI bus 0 for the northbridge and southbridge.
       *
       * Called once from the constructor. Reads the northbridge at
       * bus 0, slot 0, function 0 and scans for a southbridge ISA
       * bridge at slot 7 (Intel) or slot 1 (others). Builds the
       * device display name from the results.
       */
      void _probe();

      /**
       * @brief Looks up a vendor/device ID pair in a chipset table.
       * @param table Pointer to the first entry in the table.
       * @param tableSize Number of entries in the table.
       * @param vendorID The PCI vendor ID to match.
       * @param deviceID The PCI device ID to match.
       * @return The human-readable name if found, or `nullptr` if not.
       */
      static const char* _lookupChipset(
        const ChipsetEntry* table,
        Size tableSize,
        UInt16 vendorID,
        UInt16 deviceID
      );
  };
}
