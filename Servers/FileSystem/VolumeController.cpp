/**
 * @file Servers/FileSystem/VolumeController.cpp
 * @brief Implements @ref @QFSSrv::VolumeController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "VolumeController.hpp"

namespace Quantum::Servers::FileSystem {
  VolumeController::VolumeController(
    KernelClient& kernel,
    ServerLog& log,
    VolumeTable& volumeTable
  ) :
    RequestController(kernel, log),
    _volumeTable(volumeTable)
  {
  }

  void VolumeController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (static_cast<FileSystemOperation>(operation)) {
      case FileSystemOperation::Mount: {
        _handleMount(message);

        break;
      }

      case FileSystemOperation::Unmount: {
        _handleUnmount(message);

        break;
      }

      case FileSystemOperation::ListVolumes: {
        _handleListVolumes(message);

        break;
      }

      default: break;
    }
  }

  void VolumeController::_handleMount(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemMountRequest)) {
      _log.Warning(
        "Mount request too small (%u bytes)",
        message->PayloadSizeInBytes
      );

      return;
    }

    const FileSystemMountRequest* request
      = reinterpret_cast<const FileSystemMountRequest*>(message->Payload);
    FileSystemError error = _volumeTable.Mount(
      request->VolumeID,
      request->Label,
      request->ServicePortID,
      message->SendingProcessID,
      request->TotalBytes,
      request->UsedBytes,
      request->FreeBytes
    );
    FileSystemResult result {
      error == FileSystemError::None,
      error
    };

    SendReply(request->ReplyPortID, &result, sizeof(result));

    if (error == FileSystemError::None) {
      _log.Info(
        "Mounted volume %s \"%s\"",
        request->VolumeID,
        request->Label
      );
    } else {
      _log.Warning(
        "Mount \"%s\" failed (error %u)",
        request->VolumeID,
        static_cast<UInt32>(error)
      );
    }
  }

  void VolumeController::_handleUnmount(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemUnmountRequest)) {
      _log.Warning(
        "Unmount request too small (%u bytes)",
        message->PayloadSizeInBytes
      );

      return;
    }

    const FileSystemUnmountRequest* request
      = reinterpret_cast<const FileSystemUnmountRequest*>(message->Payload);
    FileSystemError error = _volumeTable.Unmount(request->VolumeID);
    FileSystemResult result {
      error == FileSystemError::None,
      error
    };

    SendReply(request->ReplyPortID, &result, sizeof(result));

    if (error == FileSystemError::None) {
      _log.Info(
        "Unmounted volume \"%s\"",
        request->VolumeID
      );
    } else {
      _log.Warning(
        "Unmount \"%s\" failed (error %u)",
        request->VolumeID,
        static_cast<UInt32>(error)
      );
    }
  }

  void VolumeController::_handleListVolumes(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemListVolumesRequest)) {
      _log.Warning("ListVolumes dropped: undersized message");

      return;
    }

    const FileSystemListVolumesRequest* request
      = reinterpret_cast<const FileSystemListVolumesRequest*>(message->Payload);
    constexpr Size maxVolumes = 26;
    FileSystemVolumeInfo volumes[maxVolumes];
    Size count = _volumeTable.GetActiveVolumes(volumes, maxVolumes);
    Size resultSize
      = sizeof(FileSystemListVolumesResult)
      + count * sizeof(FileSystemVolumeInfo);
    UIntPtr resultAllocation = AllocateBlock(resultSize);

    if (resultAllocation == 0) {
      FileSystemListVolumesResult errorResult {
        false,
        FileSystemError::None,
        0
      };

      SendReply(request->ReplyPortID, &errorResult, sizeof(errorResult));

      return;
    }

    FileSystemListVolumesResult* result
      = reinterpret_cast<FileSystemListVolumesResult*>(
        resultAllocation
      );

    result->Success = true;
    result->ErrorCode = FileSystemError::None;
    result->VolumeCount = static_cast<UInt32>(count);

    Byte::Copy(
      result->Volumes,
      volumes,
      count * sizeof(FileSystemVolumeInfo)
    );

    SendReply(request->ReplyPortID, result, resultSize);
    FreeBlock(resultAllocation);
  }

  void VolumeController::_sendErrorReply(
    IPCPortID replyPortID,
    FileSystemError error
  ) {
    FileSystemResult result { false, error };

    SendReply(replyPortID, &result, sizeof(result));
  }
}
