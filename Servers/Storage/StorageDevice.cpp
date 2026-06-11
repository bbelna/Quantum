/**
 * @file Servers/Storage/StorageDevice.cpp
 * @brief Implements the storage device proxy class.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StorageDevice.hpp"

namespace Quantum::Servers::Storage {
  StorageDevice::StorageDevice(
    KernelClient& kernel,
    IPCPortID driverPortID,
    UInt32 localIndex,
    const StorageDeviceDescriptor& descriptor
  ) :
    _kernel(kernel),
    _driverPortID(driverPortID),
    _localIndex(localIndex),
    _descriptor(descriptor) {}

  StorageDeviceDescriptor StorageDevice::GetDescriptor() const {
    return _descriptor;
  }

  StorageOperationErrorCode StorageDevice::ForwardRead(
    UInt64 lba,
    UInt32 count,
    Quantum::Kernel::Memory::SharedBufferID bufID,
    IPCPortResourceID replyHandle
  ) {
    DriverABI::ReadRequest req;

    req.ABIVersion = Version;
    req.Operation = DriverABI::Operation::Read;
    req.ReplyPortID = DriverABI::ReplyPortID;
    req.LocalDeviceIndex = _localIndex;
    req.LBA = lba;
    req.SectorCount = count;
    req.BufferID = bufID;

    if (!_sendToDriver(_driverPortID, &req, sizeof(req)))
      return StorageOperationErrorCode::DeviceNotReady;

    IPCMessage* reply = _kernel.ReceiveIPCMessage(replyHandle);

    if (!reply) return StorageOperationErrorCode::ReadError;

    const auto* result =
      static_cast<const DriverABI::ReadResult*>(reply->Payload);
    StorageOperationErrorCode err = result->StorageOperationErrorCode;

    free(reply);

    return err;
  }

  StorageOperationErrorCode StorageDevice::ForwardWrite(
    UInt64 lba,
    UInt32 count,
    Quantum::Kernel::Memory::SharedBufferID bufID,
    IPCPortResourceID replyHandle
  ) {
    DriverABI::WriteRequest req;

    req.ABIVersion = Version;
    req.Operation = DriverABI::Operation::Write;
    req.ReplyPortID = DriverABI::ReplyPortID;
    req.LocalDeviceIndex = _localIndex;
    req.LBA = lba;
    req.SectorCount = count;
    req.BufferID = bufID;

    if (!_sendToDriver(_driverPortID, &req, sizeof(req)))
      return StorageOperationErrorCode::DeviceNotReady;

    IPCMessage* reply = _kernel.ReceiveIPCMessage(replyHandle);

    if (!reply) return StorageOperationErrorCode::WriteError;

    const auto* result =
      static_cast<const DriverABI::WriteResult*>(reply->Payload);
    StorageOperationErrorCode err = result->StorageOperationErrorCode;

    free(reply);

    return err;
  }

  StorageOperationErrorCode StorageDevice::ForwardFlush(
    IPCPortResourceID replyHandle
  ) {
    DriverABI::FlushRequest req;

    req.ABIVersion = Version;
    req.Operation = DriverABI::Operation::Flush;
    req.ReplyPortID = DriverABI::ReplyPortID;
    req.LocalDeviceIndex = _localIndex;

    if (!_sendToDriver(_driverPortID, &req, sizeof(req)))
      return StorageOperationErrorCode::DeviceNotReady;

    IPCMessage* reply = _kernel.ReceiveIPCMessage(replyHandle);

    if (!reply) return StorageOperationErrorCode::Timeout;

    const auto* result =
      static_cast<const DriverABI::StatusResult*>(reply->Payload);
    StorageOperationErrorCode err = result->StorageOperationErrorCode;

    free(reply);

    return err;
  }

  StorageOperationErrorCode StorageDevice::ForwardGetMediaStatus(
    IPCPortResourceID replyHandle
  ) {
    DriverABI::GetMediaStatusRequest req;

    req.ABIVersion = Version;
    req.Operation = DriverABI::Operation::GetMediaStatus;
    req.ReplyPortID = DriverABI::ReplyPortID;
    req.LocalDeviceIndex = _localIndex;

    if (!_sendToDriver(_driverPortID, &req, sizeof(req)))
      return StorageOperationErrorCode::DeviceNotReady;

    IPCMessage* reply = _kernel.ReceiveIPCMessage(replyHandle);

    if (!reply) return StorageOperationErrorCode::Timeout;

    const auto* result =
      static_cast<const DriverABI::StatusResult*>(reply->Payload);
    StorageOperationErrorCode err = result->StorageOperationErrorCode;

    free(reply);

    return err;
  }

  bool StorageDevice::_sendToDriver(
    IPCPortID driverPort,
    const void* payload,
    Size payloadSize
  ) {
    IPCPortResourceID portResourceID = _kernel.OpenIPCPort(
      driverPort,
      IPCPortRights::Send
    );

    if (portResourceID == static_cast<IPCPortResourceID>(-1)) return false;

    _kernel.SendIPCMessage(portResourceID, payload, payloadSize);
    _kernel.CloseIPCPort(portResourceID);

    return true;
  }
}
