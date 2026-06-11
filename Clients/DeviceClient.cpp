/**
 * @file Clients/DeviceClient.cpp
 * @brief Implements @ref @QClients::DeviceClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "DeviceClient.hpp"

namespace Quantum::Clients {
  PointerList<Device> DeviceClient::GetDevicesInCategory(
    DeviceCategoryID categoryID
  ) {
    DeviceGetDevicesInCategoryRequest request = {};

    request.ABIVersion = DeviceABIVersion;
    request.Operation = DeviceOperation::GetDevicesInCategory;
    request.CategoryID = categoryID;

    IPCMessage* reply = InvokeOSRaw(DevicePortID, request);

    if (!reply) return PointerList<Device>();

    Size count = reply->PayloadSizeInBytes / sizeof(Device);

    return PointerList<Device>(
      static_cast<Device*>(reply->Payload),
      count
    );
  }

  bool DeviceClient::Add(const Device& device) {
    DeviceAddRequest request = {};

    request.ABIVersion = DeviceABIVersion;
    request.Operation = DeviceOperation::AddDevice;
    request.DeviceToAdd = device;

    return InvokeOS<bool, DeviceAddRequest>(
      DevicePortID,
      0,
      request
    );
  }
}
