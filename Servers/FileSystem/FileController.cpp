/**
 * @file Servers/FileSystem/FileController.cpp
 * @brief Implements @ref @QFSSrv::FileController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FileController.hpp"

namespace Quantum::Servers::FileSystem {
  FileController::FileController(
    KernelClient& kernel,
    ServerLog& log,
    VolumeTable& volumeTable,
    FileResourceRepository& resourceRepository
  ) :
    RequestController(kernel, log),
    _volumeTable(volumeTable),
    _resourceRepository(resourceRepository)
  {
  }

  void FileController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (Enum::To<FileSystemOperation>(operation)) {
      case FileSystemOperation::Open: {
        _handleOpen(message);

        break;
      }

      case FileSystemOperation::Close: {
        _handleClose(message);

        break;
      }

      case FileSystemOperation::Read: {
        _handleRead(message);

        break;
      }

      case FileSystemOperation::Write: {
        _handleWrite(message);

        break;
      }

      case FileSystemOperation::Stat: {
        _handleStat(message);

        break;
      }

      case FileSystemOperation::ReadDirectory: {
        _handleReadDirectory(message);

        break;
      }

      case FileSystemOperation::Delete: {
        _handleDelete(message);

        break;
      }

      case FileSystemOperation::CreateDirectory: {
        _handleCreateDirectory(message);

        break;
      }

      default: break;
    }
  }

  void FileController::_handleOpen(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemOpenRequest)) {
      _log.Warning("Open dropped: undersized message");

      return;
    }

    const FileSystemOpenRequest* request = Cast::As<FileSystemOpenRequest>(
      message->Payload
    );
    char label[FileSystemMaxVolumeLabelLength];
    const char* relativePath = nullptr;

    if (!_parsePath(request->Path, label, &relativePath)) {
      FileSystemOpenResult result {
        false,
        FileSystemError::InvalidPath,
        0
      };

      _log.Warning("Failed to parse \"%s\"", request->Path);

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemError resolveError = FileSystemError::VolumeNotFound;
    const Volume* volume = _volumeTable.Resolve(label, &resolveError);

    if (!volume) {
      FileSystemOpenResult result {
        false,
        resolveError,
        0
      };

      _log.Warning("Volume \"%s\" not found", label);

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemOpenRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::Open;
    forwardedRequest.ReplyPortID = 0;
    forwardedRequest.Flags = request->Flags;

    CString::Copy(
      relativePath,
      forwardedRequest.Path,
      FileSystemMaxPathLength
    );

    IPCMessage* response = _forwardAndReceive(
      volume->FileSystemPortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      FileSystemOpenResult result {
        false,
        FileSystemError::ServiceUnavailable,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    if (response->PayloadSizeInBytes >= sizeof(FileSystemOpenResult)) {
      const FileSystemOpenResult* backendResult
        = static_cast<const FileSystemOpenResult*>(
          response->Payload
        );

      if (backendResult->Success) {
        Result<FileResource*> allocateResult
          = _resourceRepository.Allocate();

        if (allocateResult.Success) {
          FileResource* resource = allocateResult.Data;

          resource->BackendHandle = backendResult->Handle;
          resource->ServicePortID = volume->FileSystemPortID;
          resource->ClientPID = message->SendingProcessID;

          FileSystemOpenResult result {
            true,
            FileSystemError::None,
            static_cast<FileHandle>(resource->ID)
          };

          SendReply(request->ReplyPortID, &result, sizeof(result));
        } else {
          FileSystemOpenResult result {
            false,
            FileSystemError::HandleTableFull,
            0
          };

          _log.Error(
            "Failed to allocate file resource for process %u",
            message->SendingProcessID
          );

          SendReply(request->ReplyPortID, &result, sizeof(result));
        }
      } else {
        SendReply(
          request->ReplyPortID,
          backendResult,
          sizeof(FileSystemOpenResult)
        );
      }
    } else {
      FileSystemOpenResult result {
        false,
        FileSystemError::ServiceUnavailable,
        0
      };

      _log.Error(
        "Invalid response size from volume \"%s\" for open request: "
        "expected at least %zu bytes, got %u bytes",
        label,
        sizeof(FileSystemOpenResult),
        response->PayloadSizeInBytes
      );

      SendReply(request->ReplyPortID, &result, sizeof(result));
    }

    delete response;
  }

  void FileController::_handleClose(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemCloseRequest)) {
      _log.Warning("Close dropped: undersized message");

      return;
    }

    const FileSystemCloseRequest* request
      = reinterpret_cast<const FileSystemCloseRequest*>(message->Payload);
    Result<FileResource*> findResult = _resourceRepository.FindFirst(
      [&](const FileResource& resource) {
        return resource.ID == static_cast<ResourceID>(request->Handle)
          && resource.ClientPID == message->SendingProcessID;
      }
    );

    if (!findResult.Success) {
      _sendErrorReply(request->ReplyPortID, FileSystemError::InvalidHandle);

      return;
    }

    FileResource* fileResource = findResult.Data;
    IPCPortID servicePortID = fileResource->ServicePortID;
    FileHandle backendHandle = fileResource->BackendHandle;
    FileSystemCloseRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::Close;
    forwardedRequest.ReplyPortID = 0;
    forwardedRequest.Handle = backendHandle;

    IPCMessage* response = _forwardAndReceive(
      servicePortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      _sendErrorReply(
        request->ReplyPortID,
        FileSystemError::ServiceUnavailable
      );

      return;
    }

    if (response->PayloadSizeInBytes >= sizeof(FileSystemResult)) {
      const FileSystemResult* backendResult
        = static_cast<const FileSystemResult*>(response->Payload);

      if (backendResult->Success) {
        _resourceRepository.Remove(fileResource->ID);
      }

      SendReply(
        request->ReplyPortID,
        backendResult,
        sizeof(FileSystemResult)
      );
    } else {
      _sendErrorReply(
        request->ReplyPortID,
        FileSystemError::ServiceUnavailable
      );
    }

    delete response;
  }

  void FileController::_handleRead(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemReadRequest)) {
      _log.Warning("Read dropped: undersized message");

      return;
    }

    const FileSystemReadRequest* request
      = reinterpret_cast<const FileSystemReadRequest*>(message->Payload);

    Result<FileResource*> findResult = _resourceRepository.FindFirst(
      [&](const FileResource& resource) {
        return resource.ID == static_cast<ResourceID>(request->Handle)
          && resource.ClientPID == message->SendingProcessID;
      }
    );

    if (!findResult.Success) {
      FileSystemReadResult result {
        false,
        FileSystemError::InvalidHandle,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileResource* fileResource = findResult.Data;
    FileSystemReadRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::Read;
    forwardedRequest.ReplyPortID = 0;
    forwardedRequest.Handle = fileResource->BackendHandle;
    forwardedRequest.Offset = request->Offset;
    forwardedRequest.Size = request->Size;

    IPCMessage* response = _forwardAndReceive(
      fileResource->ServicePortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      FileSystemReadResult result {
        false,
        FileSystemError::ServiceUnavailable,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    SendReply(
      request->ReplyPortID,
      response->Payload,
      response->PayloadSizeInBytes
    );

    delete response;
  }

  void FileController::_handleWrite(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemWriteRequest)) {
      _log.Warning("Write dropped: undersized message");

      return;
    }

    const FileSystemWriteRequest* request
      = reinterpret_cast<const FileSystemWriteRequest*>(message->Payload);

    Result<FileResource*> findResult = _resourceRepository.FindFirst(
      [&](const FileResource& resource) {
        return resource.ID == static_cast<ResourceID>(request->Handle)
          && resource.ClientPID == message->SendingProcessID;
      }
    );

    if (!findResult.Success) {
      FileSystemWriteResult result {
        false,
        FileSystemError::InvalidHandle,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileResource* fileResource = findResult.Data;
    Size forwardedSize = sizeof(FileSystemWriteRequest) + request->DataSize;
    UIntPtr forwardedAlloc = AllocateBlock(forwardedSize);

    if (forwardedAlloc == 0) {
      FileSystemWriteResult result {
        false,
        FileSystemError::IOError,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemWriteRequest* forwardedRequest
      = reinterpret_cast<FileSystemWriteRequest*>(forwardedAlloc);

    forwardedRequest->ABIVersion = FileSystemABIVersion;
    forwardedRequest->Operation = FileSystemOperation::Write;
    forwardedRequest->ReplyPortID = 0;
    forwardedRequest->Handle = fileResource->BackendHandle;
    forwardedRequest->Offset = request->Offset;
    forwardedRequest->DataSize = request->DataSize;

    Byte::Copy(
      forwardedRequest->Data,
      request->Data,
      request->DataSize
    );

    IPCMessage* response = _forwardAndReceive(
      fileResource->ServicePortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        forwardedRequest
      ),
      forwardedSize
    );

    FreeBlock(forwardedAlloc);

    if (!response) {
      FileSystemWriteResult result {
        false,
        FileSystemError::ServiceUnavailable,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    SendReply(
      request->ReplyPortID,
      response->Payload,
      response->PayloadSizeInBytes
    );

    delete response;
  }

  void FileController::_handleStat(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemStatRequest)) {
      _log.Warning("Stat dropped: undersized message");

      return;
    }

    const FileSystemStatRequest* request
      = reinterpret_cast<const FileSystemStatRequest*>(message->Payload);
    char label[FileSystemMaxVolumeLabelLength];
    const char* relativePath = nullptr;

    if (!_parsePath(request->Path, label, &relativePath)) {
      FileSystemStatResult result {
        false,
        FileSystemError::InvalidPath,
        {}
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemError resolveError = FileSystemError::VolumeNotFound;
    const Volume* volume = _volumeTable.Resolve(label, &resolveError);

    if (!volume) {
      FileSystemStatResult result {
        false,
        resolveError,
        {}
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemStatRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::Stat;
    forwardedRequest.ReplyPortID = 0;

    CString::Copy(
      relativePath,
      forwardedRequest.Path,
      FileSystemMaxPathLength
    );

    IPCMessage* response = _forwardAndReceive(
      volume->FileSystemPortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      FileSystemStatResult result {
        false,
        FileSystemError::ServiceUnavailable,
        {}
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    SendReply(
      request->ReplyPortID,
      response->Payload,
      response->PayloadSizeInBytes
    );

    delete response;
  }

  void FileController::_handleReadDirectory(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemReadDirectoryRequest)) {
      _log.Warning("ReadDirectory dropped: undersized message");

      return;
    }

    const FileSystemReadDirectoryRequest* request
      = reinterpret_cast<const FileSystemReadDirectoryRequest*>(
        message->Payload
      );
    char label[FileSystemMaxVolumeLabelLength];
    const char* relativePath = nullptr;

    if (!_parsePath(request->Path, label, &relativePath)) {
      FileSystemReadDirectoryResult result {
        false,
        FileSystemError::InvalidPath,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemError resolveError = FileSystemError::VolumeNotFound;
    const Volume* volume = _volumeTable.Resolve(label, &resolveError);

    if (!volume) {
      FileSystemReadDirectoryResult result {
        false,
        resolveError,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FileSystemReadDirectoryRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::ReadDirectory;
    forwardedRequest.ReplyPortID = 0;

    CString::Copy(
      relativePath,
      forwardedRequest.Path,
      FileSystemMaxPathLength
    );

    IPCMessage* response = _forwardAndReceive(
      volume->FileSystemPortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      FileSystemReadDirectoryResult result {
        false,
        FileSystemError::ServiceUnavailable,
        0
      };

      SendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    SendReply(
      request->ReplyPortID,
      response->Payload,
      response->PayloadSizeInBytes
    );

    delete response;
  }

  void FileController::_handleDelete(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FileSystemDeleteRequest)) {
      _log.Warning("Delete dropped: undersized message");

      return;
    }

    const FileSystemDeleteRequest* request
      = reinterpret_cast<const FileSystemDeleteRequest*>(
        message->Payload
      );
    char label[FileSystemMaxVolumeLabelLength];
    const char* relativePath = nullptr;

    if (!_parsePath(request->Path, label, &relativePath)) {
      _sendErrorReply(request->ReplyPortID, FileSystemError::InvalidPath);

      return;
    }

    FileSystemError resolveError = FileSystemError::VolumeNotFound;
    const Volume* volume = _volumeTable.Resolve(label, &resolveError);

    if (!volume) {
      _sendErrorReply(request->ReplyPortID, resolveError);

      return;
    }

    FileSystemDeleteRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::Delete;
    forwardedRequest.ReplyPortID = 0;

    CString::Copy(
      relativePath,
      forwardedRequest.Path,
      FileSystemMaxPathLength
    );

    IPCMessage* response = _forwardAndReceive(
      volume->FileSystemPortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      _sendErrorReply(
        request->ReplyPortID,
        FileSystemError::ServiceUnavailable
      );

      return;
    }

    SendReply(
      request->ReplyPortID,
      response->Payload,
      response->PayloadSizeInBytes
    );

    delete response;
  }

  void FileController::_handleCreateDirectory(const IPCMessage* message) {
    if (
      message->PayloadSizeInBytes < sizeof(FileSystemCreateDirectoryRequest)
    ) {
      _log.Warning("CreateDirectory dropped: undersized message");

      return;
    }

    const FileSystemCreateDirectoryRequest* request
      = reinterpret_cast<const FileSystemCreateDirectoryRequest*>(
        message->Payload
      );
    char label[FileSystemMaxVolumeLabelLength];
    const char* relativePath = nullptr;

    if (!_parsePath(request->Path, label, &relativePath)) {
      _sendErrorReply(request->ReplyPortID, FileSystemError::InvalidPath);

      return;
    }

    FileSystemError resolveError = FileSystemError::VolumeNotFound;
    const Volume* volume = _volumeTable.Resolve(label, &resolveError);

    if (!volume) {
      _sendErrorReply(request->ReplyPortID, resolveError);

      return;
    }

    FileSystemCreateDirectoryRequest forwardedRequest;

    forwardedRequest.ABIVersion = FileSystemABIVersion;
    forwardedRequest.Operation = FileSystemOperation::CreateDirectory;
    forwardedRequest.ReplyPortID = 0;

    CString::Copy(
      relativePath,
      forwardedRequest.Path,
      FileSystemMaxPathLength
    );

    IPCMessage* response = _forwardAndReceive(
      volume->FileSystemPortID,
      reinterpret_cast<ABIRequestWithReplyPort<FileSystemOperation>*>(
        &forwardedRequest
      ),
      sizeof(forwardedRequest)
    );

    if (!response) {
      _sendErrorReply(
        request->ReplyPortID,
        FileSystemError::ServiceUnavailable
      );

      return;
    }

    SendReply(
      request->ReplyPortID,
      response->Payload,
      response->PayloadSizeInBytes
    );

    delete response;
  }

  bool FileController::_parsePath(
    const char* fullPath,
    char* labelOut,
    const char** relativePathOut
  ) {
    if (!fullPath || !labelOut || !relativePathOut) return false;

    Size i = 0;

    while (fullPath[i] != '\0' && fullPath[i] != '/') {
      i++;
    }

    if (i == 0) return false;

    if (i >= FileSystemMaxVolumeLabelLength) return false;

    for (Size j = 0; j < i; j++) {
      labelOut[j] = fullPath[j];
    }

    labelOut[i] = '\0';

    *relativePathOut = (fullPath[i] == '/') ? &fullPath[i + 1] : &fullPath[i];

    return true;
  }

  IPCMessage* FileController::_forwardAndReceive(
    IPCPortID servicePortID,
    ABIRequestWithReplyPort<FileSystemOperation>* request,
    Size requestSize
  ) {
    IPCPortResourceID sendHandle = _kernel.OpenIPCPort(
      servicePortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) {
      _log.Error(
        "Forward failed to open send handle to port %u",
        servicePortID
      );

      return nullptr;
    }

    IPCPortResourceID replyHandle = _kernel.OpenIPCPort(
      ForwardReplyPort,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      _log.Error(
        "Forward failed to create reply port %u",
        ForwardReplyPort
      );

      _kernel.CloseIPCPort(sendHandle);

      return nullptr;
    }

    request->ReplyPortID = ForwardReplyPort;

    _kernel.SendIPCMessage(sendHandle, request, requestSize);
    _kernel.CloseIPCPort(sendHandle);

    IPCMessage* response = _kernel.ReceiveIPCMessage(replyHandle);

    _kernel.CloseIPCPort(replyHandle);

    return response;
  }

  void FileController::_sendErrorReply(
    IPCPortID replyPortID,
    FileSystemError error
  ) {
    FileSystemResult result { false, error };

    SendReply(replyPortID, &result, sizeof(result));
  }
}
