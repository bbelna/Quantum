/**
 * @file Clients/FileSystemClient.cpp
 * @brief Implements @ref @QClients::FileSystemClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "FileSystemClient.hpp"

namespace Quantum::Clients {
  Int32 FileSystemClient::ListVolumes(
    FileSystemVolumeInfo* outVolumes,
    UInt32 maxCount
  ) {
    FileSystemListVolumesRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::ListVolumes;

    IPCMessage* reply = InvokeOSRaw<FileSystemListVolumesRequest>(
      FileSystemPortID,
      request
    );
    Int32 count = -1;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(FileSystemListVolumesResult)
    ) {
      const FileSystemListVolumesResult* result
        = static_cast<const FileSystemListVolumesResult*>(
          reply->Payload
        );

      if (result->Success) {
        UInt32 toCopy = result->VolumeCount;

        if (toCopy > maxCount) toCopy = maxCount;

        Byte::Copy(
          outVolumes,
          result->Volumes,
          toCopy * sizeof(FileSystemVolumeInfo)
        );

        count = static_cast<Int32>(toCopy);
      }
    }

    if (reply) delete reply;

    return count;
  }

  bool FileSystemClient::Stat(const char* path, FileSystemFileStat* outStat) {
    FileSystemStatRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::Stat;

    CString::Copy(path, request.Path, FileSystemMaxPathLength);

    FileSystemStatResult result = InvokeOS<
      FileSystemStatResult,
      FileSystemStatRequest
    >(FileSystemPortID, 0, request);

    if (!result.Success || !outStat) return false;

    *outStat = result.Stat;

    return true;
  }

  FileHandle FileSystemClient::Open(
    const char* path,
    UInt32 flags
  ) {
    FileSystemOpenRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::Open;
    request.Flags = flags;

    CString::Copy(path, request.Path, FileSystemMaxPathLength);

    FileSystemOpenResult result = InvokeOS<
      FileSystemOpenResult,
      FileSystemOpenRequest
    >(FileSystemPortID, 0, request);

    if (!result.Success) return 0;

    return result.Handle;
  }

  bool FileSystemClient::Close(FileHandle handle) {
    FileSystemCloseRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::Close;
    request.Handle = handle;

    FileSystemResult result = InvokeOS<
      FileSystemResult,
      FileSystemCloseRequest
    >(FileSystemPortID, 0, request);

    return result.Success;
  }

  Int32 FileSystemClient::Read(
    FileHandle handle,
    void* buffer,
    UInt32 size,
    UInt32 offset
  ) {
    FileSystemReadRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::Read;
    request.Handle = handle;
    request.Offset = offset;
    request.Size = size;

    IPCMessage* reply = InvokeOSRaw<FileSystemReadRequest>(
      FileSystemPortID,
      request
    );
    Int32 bytesRead = -1;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(FileSystemReadResult)
    ) {
      const FileSystemReadResult* result
        = static_cast<const FileSystemReadResult*>(
          reply->Payload
        );

      if (result->Success && result->BytesRead > 0) {
        Byte::Copy(buffer, result->Data, result->BytesRead);

        bytesRead = static_cast<Int32>(result->BytesRead);
      } else if (result->Success) {
        bytesRead = 0;
      }
    }

    if (reply) delete reply;

    return bytesRead;
  }

  Int32 FileSystemClient::ReadDirectory(
    const char* path,
    FileSystemDirectory* entries,
    UInt32 maxEntries
  ) {
    FileSystemReadDirectoryRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::ReadDirectory;

    CString::Copy(path, request.Path, FileSystemMaxPathLength);

    IPCMessage* reply = InvokeOSRaw<FileSystemReadDirectoryRequest>(
      FileSystemPortID,
      request
    );
    Int32 count = -1;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(FileSystemReadDirectoryResult)
    ) {
      const FileSystemReadDirectoryResult* result
        = static_cast<const FileSystemReadDirectoryResult*>(
          reply->Payload
        );

      if (result->Success) {
        UInt32 toCopy = result->EntryCount;

        if (toCopy > maxEntries) toCopy = maxEntries;

        Byte::Copy(
          entries,
          result->Entries,
          toCopy * sizeof(FileSystemDirectory)
        );

        count = static_cast<Int32>(toCopy);
      }
    }

    if (reply) delete reply;

    return count;
  }

  UInt32 FileSystemClient::GetDirectorySize(const char* path) {
    UInt32 capacity = 128;
    UIntPtr directoryAddress = 0;
    Int32 entryCount = 0;

    for (;;) {
      directoryAddress = AllocateBlock(capacity * sizeof(FileSystemDirectory));

      if (directoryAddress == 0) return 0;

      FileSystemDirectory* directory
        = reinterpret_cast<FileSystemDirectory*>(
          directoryAddress
        );

      entryCount = ReadDirectory(path, directory, capacity);

      if (entryCount < static_cast<Int32>(capacity)) break;

      FreeBlock(directoryAddress);

      capacity *= 2;
    }

    if (entryCount <= 0) {
      FreeBlock(directoryAddress);

      return 0;
    }

    FileSystemDirectory* directory
      = reinterpret_cast<FileSystemDirectory*>(
        directoryAddress
      );
    UInt32 totalSize = 0;

    for (Int32 index = 0; index < entryCount; ++index) {
      if (directory[index].Type == FileSystemEntryType::Directory) {
        char childPath[FileSystemMaxPathLength];

        CString::Format(
          childPath,
          sizeof(childPath),
          "%s/%s",
          path,
          directory[index].Name
        );

        totalSize += GetDirectorySize(childPath);
      } else {
        totalSize += directory[index].Size;
      }
    }

    FreeBlock(directoryAddress);

    return totalSize;
  }

  bool FileSystemClient::CreateDirectory(const char* path) {
    FileSystemCreateDirectoryRequest request = {};

    request.ABIVersion = FileSystemABIVersion;
    request.Operation = FileSystemOperation::CreateDirectory;

    CString::Copy(path, request.Path, FileSystemMaxPathLength);

    FileSystemResult result = InvokeOS<
      FileSystemResult,
      FileSystemCreateDirectoryRequest
    >(FileSystemPortID, 0, request);

    return result.Success;
  }
}
