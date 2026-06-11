/**
 * @file Kernel/Platform/PC/Drivers/Bus/PCI.cpp
 * @brief Implements @ref @QKrnlPC::Drivers::Bus::PCI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "PCI.hpp"

namespace PCIHAL = Quantum::HAL::PCI;

namespace Quantum::Kernel::Platform::PC::Drivers::Bus {
  PCI::PCI(ICPUDriver& cpu) : _cpu(cpu) {}

  UInt32 PCI::Invoke(UInt32 operation, void* payload) {
    if (!payload) return 0;

    switch (static_cast<PCIHAL::PCIDriverOperation>(operation)) {
      case PCIHAL::PCIDriverOperation::ReadConfig: {
        auto* p = static_cast<PCIHAL::Payloads::ReadConfigPayload*>(payload);

        p->Value = ReadConfig(p->Bus, p->Slot, p->Function, p->Offset);

        return 1;
      }

      case PCIHAL::PCIDriverOperation::WriteConfig: {
        auto* p = static_cast<PCIHAL::Payloads::WriteConfigPayload*>(payload);

        WriteConfig(p->Bus, p->Slot, p->Function, p->Offset, p->Value);

        return 1;
      }

      case PCIHAL::PCIDriverOperation::FindDeviceByID: {
        auto* p = static_cast<PCIHAL::Payloads::FindDeviceByIDPayload*>(
          payload
        );

        PCIDeviceInfo info = {};

        if (!FindDevice(p->VendorID, p->DeviceID, &info)) return 0;

        _fillDeviceInfo(info.Bus, info.Slot, info.Function, &p->Result);

        return 1;
      }

      case PCIHAL::PCIDriverOperation::FindDeviceByClass: {
        auto* p = static_cast<PCIHAL::Payloads::FindDeviceByClassPayload*>(
          payload
        );

        return FindDeviceByClass(
          p->ClassCode,
          p->SubclassCode,
          &p->Result
        ) ? 1 : 0;
      }

      case PCIHAL::PCIDriverOperation::ReadBAR: {
        auto* p = static_cast<PCIHAL::Payloads::ReadBARPayload*>(payload);

        if (p->BARIndex > 5) return 0;

        p->Value = ReadBAR(p->Bus, p->Slot, p->Function, p->BARIndex);

        return 1;
      }

      case PCIHAL::PCIDriverOperation::EnableBusMastering: {
        auto* p = static_cast<PCIHAL::Payloads::EnableBusMasteringPayload*>(
          payload
        );

        // PCI Command register is at offset 0x04; bus mastering is bit 2
        UInt32 command = ReadConfig(p->Bus, p->Slot, p->Function, 0x04);

        command |= (1u << 2);

        WriteConfig(p->Bus, p->Slot, p->Function, 0x04, command);

        KLOG_TRACE(
          "Bus mastering enabled for %u:%u.%u",
          p->Bus, p->Slot, p->Function
        );

        return 1;
      }

      default: {
        return 0;
      }
    }
  }

  UInt32 PCI::ReadConfig(UInt8 bus, UInt8 slot, UInt8 func, UInt8 offset) {
    UInt32 address
      = static_cast<UInt32>(1) << 31
      | static_cast<UInt32>(bus) << 16
      | static_cast<UInt32>(slot & 0x1F) << 11
      | static_cast<UInt32>(func & 0x07) << 8
      | static_cast<UInt32>(offset & 0xFC);

    _cpu.Out32(PCIConfigAddress, address);

    return _cpu.In32(PCIConfigData);
  }

  void PCI::WriteConfig(
    UInt8 bus,
    UInt8 slot,
    UInt8 func,
    UInt8 offset,
    UInt32 value
  ) {
    UInt32 address
      = static_cast<UInt32>(1) << 31
      | static_cast<UInt32>(bus) << 16
      | static_cast<UInt32>(slot & 0x1F) << 11
      | static_cast<UInt32>(func & 0x07) << 8
      | static_cast<UInt32>(offset & 0xFC);

    _cpu.Out32(PCIConfigAddress, address);
    _cpu.Out32(PCIConfigData, value);
  }

  UInt16 PCI::ReadVendorID(UInt8 bus, UInt8 slot, UInt8 func) {
    return static_cast<UInt16>(ReadConfig(bus, slot, func, 0x00) & 0xFFFF);
  }

  UInt16 PCI::ReadDeviceID(UInt8 bus, UInt8 slot, UInt8 func) {
    return static_cast<UInt16>(
      (ReadConfig(bus, slot, func, 0x00) >> 16) & 0xFFFF
    );
  }

  UInt32 PCI::ReadBAR(UInt8 bus, UInt8 slot, UInt8 func, UInt8 barIndex) {
    if (barIndex > 5) return 0;

    UInt8 offset = 0x10 + barIndex * 4;

    return ReadConfig(bus, slot, func, offset);
  }

  UInt8 PCI::ReadHeaderType(UInt8 bus, UInt8 slot, UInt8 func) {
    return static_cast<UInt8>(
      (ReadConfig(bus, slot, func, 0x0C) >> 16) & 0xFF
    );
  }

  bool PCI::FindDevice(UInt16 vendorID, UInt16 deviceID, PCIDeviceInfo* out) {
    for (UInt8 slot = 0; slot < 32; ++slot) {
      UInt16 vendor = ReadVendorID(0, slot, 0);

      if (vendor == 0xFFFF) continue;

      UInt8 headerType = ReadHeaderType(0, slot, 0);
      UInt8 maxFunctions = (headerType & 0x80) ? 8 : 1;

      for (UInt8 func = 0; func < maxFunctions; ++func) {
        if (func > 0) {
          vendor = ReadVendorID(0, slot, func);

          if (vendor == 0xFFFF) continue;
        }

        UInt16 device = ReadDeviceID(0, slot, func);

        if (vendor == vendorID && device == deviceID) {
          if (out) {
            out->Bus = 0;
            out->Slot = slot;
            out->Function = func;
            out->VendorID = vendorID;
            out->DeviceID = deviceID;

            for (UInt8 i = 0; i < 6; ++i) {
              out->BAR[i] = ReadBAR(0, slot, func, i);
            }
          }

          return true;
        }
      }
    }

    return false;
  }

  bool PCI::FindDeviceByClass(
    UInt8 classCode,
    UInt8 subclassCode,
    HAL::PCI::PCIDeviceInfo* out
  ) {
    UInt16 target = (static_cast<UInt16>(classCode) << 8) | subclassCode;

    for (UInt8 slot = 0; slot < 32; ++slot) {
      UInt16 vendor = ReadVendorID(0, slot, 0);

      if (vendor == 0xFFFF) continue;

      UInt8 headerType = ReadHeaderType(0, slot, 0);
      UInt8 maxFunctions = (headerType & 0x80) ? 8 : 1;

      for (UInt8 func = 0; func < maxFunctions; ++func) {
        if (func > 0) {
          vendor = ReadVendorID(0, slot, func);

          if (vendor == 0xFFFF) continue;
        }

        UInt32 classRegister = ReadConfig(0, slot, func, 0x08);
        UInt16 deviceClass = static_cast<UInt16>(classRegister >> 16);

        if (deviceClass == target) {
          if (out) _fillDeviceInfo(0, slot, func, out);

          return true;
        }
      }
    }

    return false;
  }

  void PCI::_fillDeviceInfo(
    UInt8 bus,
    UInt8 slot,
    UInt8 func,
    HAL::PCI::PCIDeviceInfo* out
  ) {
    out->Bus = bus;
    out->Slot = slot;
    out->Function = func;
    out->VendorID = ReadVendorID(bus, slot, func);
    out->DeviceID = ReadDeviceID(bus, slot, func);

    UInt32 classRegister = ReadConfig(bus, slot, func, 0x08);

    out->ClassCode = static_cast<UInt8>((classRegister >> 24) & 0xFF);
    out->SubclassCode = static_cast<UInt8>((classRegister >> 16) & 0xFF);

    for (UInt8 i = 0; i < 6; ++i) {
      out->BAR[i] = ReadBAR(bus, slot, func, i);
    }
  }
}
