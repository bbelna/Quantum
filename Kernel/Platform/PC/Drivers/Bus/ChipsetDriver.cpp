/**
 * @file Kernel/Platform/PC/Drivers/Bus/ChipsetDriver.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Bus::ChipsetDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "ChipsetDriver.hpp"

namespace Quantum::Kernel::Platform::PC::Drivers::Bus {
  ChipsetDriver::ChipsetDriver(PCI* pci) : _pci(pci) {
    _probe();
  }

  UInt32 ChipsetDriver::Invoke(UInt32 operation, void* payload) {
    return 0;
  }

  void ChipsetDriver::_probe() {
    // read northbridge at bus 0, slot 0, function 0
    UInt16 northbridgeVendorID = _pci->ReadVendorID(0, 0, 0);
    UInt16 northbridgeDeviceID = _pci->ReadDeviceID(0, 0, 0);

    if (northbridgeVendorID == 0xFFFF) {
      KLOG_INFO("No device at bus 0 slot 0");

      return;
    }

    const char* northbridgeName = _lookupChipset(
      NorthbridgeTable,
      sizeof(NorthbridgeTable) / sizeof(NorthbridgeTable[0]),
      northbridgeVendorID,
      northbridgeDeviceID
    );

    // store the northbridge PCI address in the device
    SetPCIAddress(_device, { 0, 0, 0 });

    // scan for southbridge: try slot 7 functions 0-3 first (Intel convention),
    // then fall back to slot 1 functions 0-3 (VIA and others)
    const char* southbridgeName = nullptr;
    UInt16 southbridgeVendorID = 0xFFFF;
    UInt16 southbridgeDeviceID = 0;

    static constexpr UInt8 southbridgeSlots[] = { 7, 1 };

    for (UInt8 slot : southbridgeSlots) {
      for (UInt8 function = 0; function < 4; ++function) {
        UInt16 candidateVendorID = _pci->ReadVendorID(0, slot, function);

        if (candidateVendorID == 0xFFFF) continue;

        // read class code (offset 0x08, upper 16 bits = class + subclass)
        UInt32 classRegister = _pci->ReadConfig(0, slot, function, 0x08);
        UInt16 classCode = static_cast<UInt16>(classRegister >> 16);

        if (classCode == PCIClassISABridge) {
          southbridgeVendorID = candidateVendorID;
          southbridgeDeviceID = _pci->ReadDeviceID(0, slot, function);

          southbridgeName = _lookupChipset(
            SouthbridgeTable,
            sizeof(SouthbridgeTable) / sizeof(SouthbridgeTable[0]),
            southbridgeVendorID,
            southbridgeDeviceID
          );

          break;
        }
      }

      if (southbridgeName) break;
    }

    // build the display name
    if (northbridgeName && southbridgeName) {
      CString::Format(
        _device.DisplayName,
        DeviceDisplayNameMaxLength,
        "%s + %s",
        northbridgeName,
        southbridgeName
      );
    } else if (northbridgeName) {
      CString::Format(
        _device.DisplayName,
        DeviceDisplayNameMaxLength,
        "%s",
        northbridgeName
      );
    } else {
      CString::Format(
        _device.DisplayName,
        DeviceDisplayNameMaxLength,
        "PCI %x:%x",
        northbridgeVendorID,
        northbridgeDeviceID
      );
    }

    KLOG_TRACE("Chipset is %s", _device.DisplayName);
  }

  const char* ChipsetDriver::_lookupChipset(
    const ChipsetEntry* table,
    Size tableSize,
    UInt16 vendorID,
    UInt16 deviceID
  ) {
    for (Size index = 0; index < tableSize; ++index) {
      if (
        table[index].VendorID == vendorID &&
        table[index].DeviceID == deviceID
      ) return table[index].Name;
    }

    return nullptr;
  }
}
