/**
 * @file Clients/PCIDriverClient.cpp
 * @brief Implements @ref @QClients::PCIDriverClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "PCIDriverClient.hpp"

namespace Quantum::Clients {
  UInt32 PCIDriverClient::ReadConfig(
    UInt8 bus,
    UInt8 slot,
    UInt8 function,
    UInt8 offset
  ) {
    HAL::PCI::Payloads::ReadConfigPayload payload = {};

    payload.Bus = bus;
    payload.Slot = slot;
    payload.Function = function;
    payload.Offset = offset;

    InvokeDriver(
      Enum::ToBase(HAL::PCI::PCIDriverOperation::ReadConfig),
      &payload
    );

    return payload.Value;
  }

  void PCIDriverClient::WriteConfig(
    UInt8 bus,
    UInt8 slot,
    UInt8 function,
    UInt8 offset,
    UInt32 value
  ) {
    HAL::PCI::Payloads::WriteConfigPayload payload = {};

    payload.Bus = bus;
    payload.Slot = slot;
    payload.Function = function;
    payload.Offset = offset;
    payload.Value = value;

    InvokeDriver(
      Enum::ToBase(HAL::PCI::PCIDriverOperation::WriteConfig),
      &payload
    );
  }

  bool PCIDriverClient::FindDeviceByID(
    UInt16 vendorID,
    UInt16 deviceID,
    HAL::PCI::PCIDeviceInfo& result
  ) {
    HAL::PCI::Payloads::FindDeviceByIDPayload payload = {};

    payload.VendorID = vendorID;
    payload.DeviceID = deviceID;

    UInt32 found = InvokeDriver(
      Enum::ToBase(HAL::PCI::PCIDriverOperation::FindDeviceByID),
      &payload
    );

    if (found) result = payload.Result;

    return found != 0;
  }

  bool PCIDriverClient::FindDeviceByClass(
    UInt8 classCode,
    UInt8 subclassCode,
    HAL::PCI::PCIDeviceInfo& result
  ) {
    HAL::PCI::Payloads::FindDeviceByClassPayload payload = {};

    payload.ClassCode = classCode;
    payload.SubclassCode = subclassCode;

    UInt32 found = InvokeDriver(
      Enum::ToBase(HAL::PCI::PCIDriverOperation::FindDeviceByClass),
      &payload
    );

    if (found) result = payload.Result;

    return found != 0;
  }

  UInt32 PCIDriverClient::ReadBAR(
    UInt8 bus,
    UInt8 slot,
    UInt8 function,
    UInt8 barIndex
  ) {
    HAL::PCI::Payloads::ReadBARPayload payload = {};

    payload.Bus = bus;
    payload.Slot = slot;
    payload.Function = function;
    payload.BARIndex = barIndex;

    InvokeDriver(
      Enum::ToBase(HAL::PCI::PCIDriverOperation::ReadBAR),
      &payload
    );

    return payload.Value;
  }

  void PCIDriverClient::EnableBusMastering(
    UInt8 bus,
    UInt8 slot,
    UInt8 function
  ) {
    HAL::PCI::Payloads::EnableBusMasteringPayload payload = {};

    payload.Bus = bus;
    payload.Slot = slot;
    payload.Function = function;

    InvokeDriver(
      Enum::ToBase(HAL::PCI::PCIDriverOperation::EnableBusMastering),
      &payload
    );
  }
}
