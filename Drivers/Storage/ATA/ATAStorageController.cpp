/**
 * @file Drivers/Storage/ATA/ATAStorageController.cpp
 * @brief Implements @ref @QDrvs::Storage::ATA::ATAStorageController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ATAStorageController.hpp"

namespace Quantum::Drivers::Storage::ATA {
  ATAStorageController::ATAStorageController(
    KernelClient& kernel,
    ServerLog& log,
    ATADriver& driver
  ) : RequestController(kernel, log), _driver(driver) {}

  void ATAStorageController::Handle(
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

  void ATAStorageController::_handleRead(const IPCMessage* message) {
    const auto* request = static_cast<const DriverABI::ReadRequest*>(
      message->Payload
    );

    UIntPtr virtualAddress = _kernel.AttachSharedBuffer(request->BufferID);

    DriverABI::ReadResult result;

    if (virtualAddress == 0) {
      result.Success = false;
      result.StorageOperationErrorCode = StorageABI::StorageOperationErrorCode::ReadError;
      result.BytesRead = 0;
    } else {
      StorageABI::StorageOperationErrorCode error = _driver.Read(
        static_cast<UInt8>(request->LocalDeviceIndex),
        request->LBA,
        request->SectorCount,
        reinterpret_cast<void*>(virtualAddress)
      );

      _kernel.DetachSharedBuffer(virtualAddress);

      result.Success = (error == StorageABI::StorageOperationErrorCode::None);
      result.StorageOperationErrorCode = error;
      result.BytesRead = result.Success ? request->SectorCount * 512 : 0;
    }

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void ATAStorageController::_handleWrite(const IPCMessage* message) {
    const auto* request = static_cast<const DriverABI::WriteRequest*>(
      message->Payload
    );

    UIntPtr virtualAddress = _kernel.AttachSharedBuffer(request->BufferID);

    DriverABI::WriteResult result;

    if (virtualAddress == 0) {
      result.Success = false;
      result.StorageOperationErrorCode = StorageABI::StorageOperationErrorCode::WriteError;
      result.BytesWritten = 0;
    } else {
      StorageABI::StorageOperationErrorCode error = _driver.Write(
        static_cast<UInt8>(request->LocalDeviceIndex),
        request->LBA,
        request->SectorCount,
        reinterpret_cast<const void*>(virtualAddress)
      );

      _kernel.DetachSharedBuffer(virtualAddress);

      result.Success = (error == StorageABI::StorageOperationErrorCode::None);
      result.StorageOperationErrorCode = error;
      result.BytesWritten = result.Success
        ? request->SectorCount * 512
        : 0;
    }

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void ATAStorageController::_handleFlush(const IPCMessage* message) {
    const auto* request = static_cast<const DriverABI::FlushRequest*>(
      message->Payload
    );

    StorageABI::StorageOperationErrorCode error = _driver.Flush(
      static_cast<UInt8>(request->LocalDeviceIndex)
    );

    DriverABI::StatusResult result;

    result.Success = (error == StorageABI::StorageOperationErrorCode::None);
    result.StorageOperationErrorCode = error;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void ATAStorageController::_handleGetMediaStatus(
    const IPCMessage* message
  ) {
    const auto* request = static_cast<
      const DriverABI::GetMediaStatusRequest*
    >(message->Payload);

    UInt8 index = static_cast<UInt8>(request->LocalDeviceIndex);
    bool present = (index < 4) && _driver.GetDriveInfo(index).Present;

    DriverABI::StatusResult result;

    result.Success = present;
    result.StorageOperationErrorCode = present
      ? StorageABI::StorageOperationErrorCode::None
      : StorageABI::StorageOperationErrorCode::NoMedia;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }
}
