/**
 * @file Drivers/Storage/FDC/FDCStorageController.cpp
 * @brief Implements @ref @QDrvs::Storage::FDC::FDCStorageController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FDCStorageController.hpp"

namespace Quantum::Drivers::Storage::FDC {
  FDCStorageController::FDCStorageController(
    KernelClient& kernel,
    ServerLog& log,
    FDCDriver& driver
  ) : RequestController(kernel, log), _driver(driver) {}

  void FDCStorageController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (static_cast<DriverABI::Operation>(operation)) {
      case DriverABI::Operation::Read: {
        _handleRead(message);

        break;
      } case DriverABI::Operation::Write: {
        _handleWrite(message);

        break;
      } case DriverABI::Operation::Flush: {
        _handleFlush(message);

        break;
      } case DriverABI::Operation::GetMediaStatus: {
        _handleGetMediaStatus(message);

        break;
      } default: {
        break;
      }
    }
  }

  void FDCStorageController::SetDeviceMapping(
    UInt32 deviceCount,
    const UInt8* driveNumbers
  ) {
    _deviceCount = deviceCount;

    for (UInt32 i = 0; i < deviceCount && i < 2; i++) {
      _driveNumbers[i] = driveNumbers[i];
    }
  }

  void FDCStorageController::_handleRead(const IPCMessage* message) {
    const auto* request = static_cast<const DriverABI::ReadRequest*>(
      message->Payload
    );

    UInt8 driveIndex = (request->LocalDeviceIndex < _deviceCount)
      ? _driveNumbers[request->LocalDeviceIndex]
      : 0;

    UIntPtr virtualAddress = _kernel.AttachSharedBuffer(request->BufferID);

    DriverABI::ReadResult result;

    if (virtualAddress == 0) {
      result.Success = false;
      result.StorageOperationErrorCode = StorageABI::StorageOperationErrorCode::ReadError;
      result.BytesRead = 0;
    } else {
      StorageABI::StorageOperationErrorCode error = _driver.Read(
        driveIndex,
        request->LBA,
        request->SectorCount,
        reinterpret_cast<void*>(virtualAddress)
      );

      _kernel.DetachSharedBuffer(virtualAddress);

      result.Success = (error == StorageABI::StorageOperationErrorCode::None);
      result.StorageOperationErrorCode = error;
      result.BytesRead = result.Success
        ? request->SectorCount * FDCDriver::SectorSize
        : 0;
    }

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void FDCStorageController::_handleWrite(const IPCMessage* message) {
    const auto* request = static_cast<const DriverABI::WriteRequest*>(
      message->Payload
    );

    UInt8 driveIndex = (request->LocalDeviceIndex < _deviceCount)
      ? _driveNumbers[request->LocalDeviceIndex]
      : 0;

    UIntPtr virtualAddress = _kernel.AttachSharedBuffer(request->BufferID);

    DriverABI::WriteResult result;

    if (virtualAddress == 0) {
      result.Success = false;
      result.StorageOperationErrorCode = StorageABI::StorageOperationErrorCode::WriteError;
      result.BytesWritten = 0;
    } else {
      StorageABI::StorageOperationErrorCode error = _driver.Write(
        driveIndex,
        request->LBA,
        request->SectorCount,
        reinterpret_cast<const void*>(virtualAddress)
      );

      _kernel.DetachSharedBuffer(virtualAddress);

      result.Success = (error == StorageABI::StorageOperationErrorCode::None);
      result.StorageOperationErrorCode = error;
      result.BytesWritten = result.Success
        ? request->SectorCount * FDCDriver::SectorSize
        : 0;
    }

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void FDCStorageController::_handleFlush(const IPCMessage* message) {
    const auto* request = static_cast<const DriverABI::FlushRequest*>(
      message->Payload
    );

    // Floppy disk has no write-back cache.
    DriverABI::StatusResult result;

    result.Success = true;
    result.StorageOperationErrorCode = StorageABI::StorageOperationErrorCode::None;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void FDCStorageController::_handleGetMediaStatus(
    const IPCMessage* message
  ) {
    const auto* request = static_cast<
      const DriverABI::GetMediaStatusRequest*
    >(message->Payload);

    UInt8 driveIndex = (request->LocalDeviceIndex < _deviceCount)
      ? _driveNumbers[request->LocalDeviceIndex]
      : 0;

    bool present = _driver.DrivePresent(driveIndex);

    DriverABI::StatusResult result;

    result.Success = present;
    result.StorageOperationErrorCode = present
      ? StorageABI::StorageOperationErrorCode::None
      : StorageABI::StorageOperationErrorCode::NoMedia;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }
}
