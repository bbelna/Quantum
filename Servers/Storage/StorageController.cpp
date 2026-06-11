/**
 * @file Servers/Storage/StorageController.cpp
 * @brief Implements @ref @QStrSrv::StorageController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StorageController.hpp"

namespace Quantum::Servers::Storage {
  StorageController::StorageController(
    KernelClient& kernel,
    ServerLog& log,
    StorageDeviceRegistry& registry,
    IPCPortResourceID driverReplyHandle
  ) :
    RequestController(kernel, log),
    _registry(registry),
    _driverReplyHandle(driverReplyHandle)
  {
  }

  void StorageController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (Enum::To<StorageOperation>(operation)) {
      case StorageOperation::RegisterDriver: {
        _handleRegisterDriver(message);

        break;
      }

      case StorageOperation::GetDevices: {
        _handleGetDevices(message);

        break;
      }

      case StorageOperation::GetDeviceInfo: {
        _handleGetDeviceInfo(message);

        break;
      }

      case StorageOperation::Read: {
        _handleRead(message);

        break;
      }

      case StorageOperation::Write: {
        _handleWrite(message);

        break;
      }

      case StorageOperation::Flush: {
        _handleFlush(message);

        break;
      }

      case StorageOperation::GetMediaStatus: {
        _handleGetMediaStatus(message);

        break;
      }

      default: {
        _log.Warning(
          "Unknown operation %u",
          operation
        );

        break;
      }
    }
  }

  void StorageController::_registerWithDeviceServer(
    const StorageDevice* device
  ) {
    StorageDeviceDescriptor deviceDescriptor = device->GetDescriptor();

    Quantum::HAL::Device deviceEntry;

    deviceEntry.ID = 0;
    deviceEntry.CategoryID = ToDeviceCategoryID(DeviceCategoryType::Storage);
    deviceEntry.State = DeviceState::Active;
    deviceEntry.DriverPID = _kernel.GetProcessID();
    deviceEntry.ParentID = 0;
    deviceEntry.Bus = DeviceBus::ISA;
    deviceEntry.BusDataSize = 0;

    UInt32 deviceNameIndex = 0;

    while (
      deviceDescriptor.Name[deviceNameIndex] &&
      deviceNameIndex < DeviceNameMaxLength - 1
    ) {
      deviceEntry.Name[deviceNameIndex]
        = deviceDescriptor.Name[deviceNameIndex];
      deviceNameIndex++;
    }

    deviceEntry.Name[deviceNameIndex] = '\0';

    UInt32 deviceDisplayNameIndex = 0;

    while (
      deviceDescriptor.DisplayName[deviceDisplayNameIndex] &&
      deviceDisplayNameIndex < DeviceDisplayNameMaxLength - 1
    ) {
      deviceEntry.DisplayName[deviceDisplayNameIndex]
        = deviceDescriptor.DisplayName[deviceDisplayNameIndex];
      deviceDisplayNameIndex++;
    }

    deviceEntry.DisplayName[deviceDisplayNameIndex] = '\0';

    DeviceClient deviceClient;

    deviceClient.Add(deviceEntry);
  }

  void StorageController::_handleRegisterDriver(
    const IPCMessage* message
  ) {
    if (message->PayloadSizeInBytes < sizeof(RegisterDriverRequest)) {
      _log.Warning("RegisterDriver dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const RegisterDriverRequest*>(
      message->Payload
    );

    for (
      UInt32 i = 0;
      i < request->DeviceCount && i < MaxDevicesPerDriver;
      i++
    ) {
      auto* device = new StorageDevice(
        _kernel,
        request->DriverPortID,
        i,
        request->Devices[i]
      );

      if (!_registry.Register(device)) {
        delete device;

        continue;
      }

      _registerWithDeviceServer(device);
    }
  }

  void StorageController::_handleGetDevices(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(GetDevicesRequest)) {
      _log.Warning("GetDevices dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const GetDevicesRequest*>(
      message->Payload
    );

    GetDevicesResult result;

    result.Count = _registry.Count();

    for (UInt32 i = 0; i < result.Count && i < MaxDevices; i++)
      result.Devices[i] = _registry.GetAt(i)->GetDescriptor();

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void StorageController::_handleGetDeviceInfo(
    const IPCMessage* message
  ) {
    if (message->PayloadSizeInBytes < sizeof(GetDeviceInfoRequest)) {
      _log.Warning("GetDeviceInfo dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const GetDeviceInfoRequest*>(
      message->Payload
    );

    StorageDevice* device = _registry.Find(request->DeviceID);

    if (!device) {
      GetDeviceInfoResult result;

      result.Success = false;
      result.StorageOperationErrorCode
        = StorageOperationErrorCode::InvalidDevice;

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    GetDeviceInfoResult result;

    result.Success = true;
    result.StorageOperationErrorCode = StorageOperationErrorCode::None;
    result.Descriptor = device->GetDescriptor();

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void StorageController::_handleRead(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(ReadRequest)) {
      _log.Warning("Read dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const ReadRequest*>(
      message->Payload
    );

    StorageDevice* device = _registry.Find(request->DeviceID);

    if (!device) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidDevice
      );

      return;
    }

    StorageDeviceDescriptor deviceDescriptor = device->GetDescriptor();

    if (request->LBA >= deviceDescriptor.SectorCount) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidLBA
      );

      return;
    }

    if (
      request->SectorCount == 0 ||
      request->LBA + request->SectorCount > deviceDescriptor.SectorCount
    ) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidCount
      );

      return;
    }

    StorageOperationErrorCode err = device->ForwardRead(
      request->LBA,
      request->SectorCount,
      request->BufferID,
      _driverReplyHandle
    );

    ReadResult result;

    result.Success = (err == StorageOperationErrorCode::None);
    result.StorageOperationErrorCode = err;
    result.BytesRead = result.Success
      ? request->SectorCount * deviceDescriptor.SectorSize
      : 0;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void StorageController::_handleWrite(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(WriteRequest)) {
      _log.Warning("Write dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const WriteRequest*>(
      message->Payload
    );

    StorageDevice* device = _registry.Find(request->DeviceID);

    if (!device) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidDevice
      );

      return;
    }

    StorageDeviceDescriptor deviceDescriptor = device->GetDescriptor();

    if (deviceDescriptor.ReadOnly) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::WriteProtected
      );

      return;
    }

    if (request->LBA >= deviceDescriptor.SectorCount) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidLBA
      );

      return;
    }

    if (
      request->SectorCount == 0 ||
      request->LBA + request->SectorCount > deviceDescriptor.SectorCount
    ) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidCount
      );

      return;
    }

    StorageOperationErrorCode err = device->ForwardWrite(
      request->LBA,
      request->SectorCount,
      request->BufferID,
      _driverReplyHandle
    );

    WriteResult result;

    result.Success = (err == StorageOperationErrorCode::None);
    result.StorageOperationErrorCode = err;
    result.BytesWritten = result.Success
      ? request->SectorCount * deviceDescriptor.SectorSize
      : 0;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void StorageController::_handleFlush(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FlushRequest)) {
      _log.Warning("Flush dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const FlushRequest*>(
      message->Payload
    );

    StorageDevice* device = _registry.Find(request->DeviceID);

    if (!device) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidDevice
      );

      return;
    }

    StorageOperationErrorCode err = device->ForwardFlush(
      _driverReplyHandle
    );

    StatusResult result;

    result.Success = (err == StorageOperationErrorCode::None);
    result.StorageOperationErrorCode = err;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void StorageController::_handleGetMediaStatus(
    const IPCMessage* message
  ) {
    if (message->PayloadSizeInBytes < sizeof(GetMediaStatusRequest)) {
      _log.Warning("GetMediaStatus dropped: undersized message");

      return;
    }

    const auto* request = static_cast<const GetMediaStatusRequest*>(
      message->Payload
    );

    StorageDevice* device = _registry.Find(request->DeviceID);

    if (!device) {
      _sendErrorReply(
        request->ReplyPortID,
        StorageOperationErrorCode::InvalidDevice
      );

      return;
    }

    StorageOperationErrorCode err = device->ForwardGetMediaStatus(
      _driverReplyHandle
    );

    StatusResult result;

    result.Success = (err == StorageOperationErrorCode::None);
    result.StorageOperationErrorCode = err;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void StorageController::_sendErrorReply(
    IPCPortID portID,
    StorageOperationErrorCode error
  ) {
    StatusResult result;

    result.Success = false;
    result.StorageOperationErrorCode = error;

    SendReply(portID, &result, sizeof(result));
  }
}
