/**
 * @file Servers/Device/DeviceController.cpp
 * @brief Implements @ref @QDvSrv::DeviceController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "DeviceController.hpp"

namespace Quantum::Servers::Device {
  DeviceController::DeviceController(
    KernelClient& kernel,
    ServerLog& log,
    DeviceManager& deviceManager
  ) : RequestController(kernel, log), _deviceManager(deviceManager) {}

  void DeviceController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (Enum::To<DeviceOperation>(operation)) {
      case DeviceOperation::GetDevicesInCategory: {
        _handleGetDevicesInCategory(message);

        break;
      }

      case DeviceOperation::AddDevice: {
        _handleAddDevice(message);

        break;
      }

      default: {
        _log.Write(
          LogLevel::Warning,
          "Unknown operation %u",
          operation
        );

        break;
      }
    }
  }

  void DeviceController::_handleGetDevicesInCategory(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(DeviceGetDevicesInCategoryRequest)
    ) {
      _log.Write(
        LogLevel::Warning,
        "GetDevicesInCategory request too small"
      );

      return;
    }

    const DeviceGetDevicesInCategoryRequest* request =
      reinterpret_cast<const DeviceGetDevicesInCategoryRequest*>(
        message->Payload
      );

    PointerList<Device> devices = _deviceManager.GetInCategory(
      request->CategoryID
    );

    SendReply(
      request->ReplyPortID,
      devices.GetCount() > 0
        ? static_cast<const void*>(&devices[0])
        : nullptr,
      devices.GetCount() * sizeof(Device)
    );

    if (devices.GetCount() > 0) {
      delete[] &devices[0];
    }
  }

  void DeviceController::_handleAddDevice(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(DeviceAddRequest)) {
      _log.Write(
        LogLevel::Warning,
        "AddDevice request too small"
      );

      return;
    }

    const DeviceAddRequest* request =
      reinterpret_cast<const DeviceAddRequest*>(
        message->Payload
      );
    bool success = _deviceManager.Add(request->DeviceToAdd);

    if (!success) {
      _log.Write(
        LogLevel::Warning,
        "Failed to add device %s",
        request->DeviceToAdd.Name
      );
    }

    SendReply(
      request->ReplyPortID,
      static_cast<const void*>(&success),
      sizeof(success)
    );
  }
}
