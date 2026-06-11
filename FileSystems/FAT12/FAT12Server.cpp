/**
 * @file FileSystems/FAT12/FAT12Server.cpp
 * @brief Implements the FAT12 file system backend server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core/Byte.hpp>
#include <Quantum/Core/CString.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Servers/FileSystem/ABI.hpp>
#include <Quantum/Clients/StartupClient.hpp>
#include <Quantum/Runtime.hpp>
#include <Quantum/Servers/Storage/ABI.hpp>
#include <Quantum/Threading/Thread.hpp>

#include "FAT12Server.hpp"

using namespace Quantum::Kernel;
using namespace Quantum::Kernel::ABI;

namespace FSABI   = ::Quantum::Servers::FileSystem::ABI;
namespace StorABI = ::Quantum::Servers::Storage::ABI;
namespace KernelLog = Quantum::Kernel::ABI::Log;

namespace Quantum::FileSystems::FAT12 {
  void FAT12Server::Start(UInt32 deviceID, const char* deviceName) {
    // open an auto-assigned IPC port for receiving forwarded FS operations
    IPCPortID myPortID = static_cast<IPCPortID>(-1);

    IPCPortResourceID handle = Kernel::ABI::IPC::Open(
      static_cast<IPCPortID>(-1),
      IPCPortRights::Manage | IPCPortRights::Receive,
      &myPortID
    );

    if (handle == static_cast<IPCPortResourceID>(-1)) {
      KernelLog::Write(Core::LogLevel::Error, "Failed to open IPC port");

      return;
    }

    // Mount the FAT12 file system on that device.
    if (!_volume.Mount(deviceID)) {
      KernelLog::Write(Core::LogLevel::Error, "Failed to mount volume");

      Kernel::ABI::IPC::Close(handle);

      return;
    }

    const char* volumeLabel = _volume.GetLabel();

    // Register this volume with the FileSystem server.
    UInt32 totalBytes = _volume.GetTotalBytes();
    UInt32 freeBytes = _volume.GetFreeBytes();
    UInt32 usedBytes = totalBytes - freeBytes;

    FSABI::FileSystemMountRequest mountRequest = {};

    mountRequest.ABIVersion = FSABI::FileSystemABIVersion;
    mountRequest.Operation = FSABI::FileSystemOperation::Mount;
    mountRequest.ServicePortID = myPortID;
    mountRequest.TotalBytes = totalBytes;
    mountRequest.UsedBytes = usedBytes;
    mountRequest.FreeBytes = freeBytes;

    Quantum::Core::CString::Copy(
      deviceName, mountRequest.VolumeID, FSABI::FileSystemMaxVolumeIDLength
    );
    Quantum::Core::CString::Copy(
      volumeLabel, mountRequest.Label, FSABI::FileSystemMaxVolumeLabelLength
    );

    FSABI::FileSystemResult mountResult = InvokeOS<
      FSABI::FileSystemResult,
      FSABI::FileSystemMountRequest
    >(FSABI::FileSystemPortID, 0, mountRequest);

    if (!mountResult.Success) {
      KernelLog::Write(Core::LogLevel::Error, "FileSystem::Mount failed");
      Kernel::ABI::IPC::Close(handle);

      return;
    }

    // Signal startup completion.
    Quantum::Clients::StartupClient startup;
    startup.Ready();

    // Message loop.
    for (;;) {
      IPCMessage* message = Kernel::ABI::IPC::Receive(handle);

      if (!message) continue;

      if (message->PayloadSizeInBytes < sizeof(FSABI::FileSystemRequest)) {
        free(message);

        continue;
      }

      const auto* req = reinterpret_cast<const FSABI::FileSystemRequest*>(
        message->Payload
      );

      switch (req->Operation) {
        case FSABI::FileSystemOperation::Open: {
          _handleOpen(message);

          break;
        } case FSABI::FileSystemOperation::Close: {
          _handleClose(message);

          break;
        } case FSABI::FileSystemOperation::Read: {
          _handleRead(message);

          break;
        } case FSABI::FileSystemOperation::Stat: {
          _handleStat(message);

          break;
        } case FSABI::FileSystemOperation::ReadDirectory: {
          _handleReadDirectory(message);

          break;
        } default: {
          if (message->PayloadSizeInBytes >=
              sizeof(ABIRequestWithReplyPort<FSABI::FileSystemOperation>)) {
            const auto* base = reinterpret_cast<
              const ABIRequestWithReplyPort<FSABI::FileSystemOperation>*>(
                message->Payload
            );

            _sendInvalidOperation(base->ReplyPortID);
          }

          break;
        }
      }

      free(message);
    }
  }

  void FAT12Server::_handleOpen(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FSABI::FileSystemOpenRequest)) return;

    const auto* request = reinterpret_cast<const FSABI::FileSystemOpenRequest*>(
      message->Payload
    );

    ResolvedEntry entry;

    if (!_volume.FindEntry(request->Path, &entry)) {
      FSABI::FileSystemOpenResult result { false, FSABI::FileSystemError::FileNotFound, 0 };

      _sendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    if (entry.IsDirectory) {
      FSABI::FileSystemOpenResult result { false, FSABI::FileSystemError::IsADirectory, 0 };

      _sendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FSABI::FileHandle h = _handles.Open(entry);

    if (h == 0) {
      FSABI::FileSystemOpenResult result { false, FSABI::FileSystemError::HandleTableFull, 0 };

      _sendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FSABI::FileSystemOpenResult result { true, FSABI::FileSystemError::None, h };

    _sendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void FAT12Server::_handleClose(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FSABI::FileSystemCloseRequest)) return;

    const auto* request = reinterpret_cast<const FSABI::FileSystemCloseRequest*>(
      message->Payload
    );

    if (!_handles.Get(request->Handle)) {
      FSABI::FileSystemResult result { false, FSABI::FileSystemError::InvalidHandle };

      _sendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    _handles.Close(request->Handle);

    FSABI::FileSystemResult result { true, FSABI::FileSystemError::None };

    _sendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void FAT12Server::_handleRead(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FSABI::FileSystemReadRequest)) return;

    const auto* request = reinterpret_cast<const FSABI::FileSystemReadRequest*>(
      message->Payload
    );

    OpenFile* file = _handles.Get(request->Handle);

    if (!file) {
      FSABI::FileSystemReadResult errResult { false, FSABI::FileSystemError::InvalidHandle, 0 };

      _sendReply(request->ReplyPortID, &errResult, sizeof(errResult));

      return;
    }

    // allocate a result buffer: fixed header + inline read data
    Size resultSize = sizeof(FSABI::FileSystemReadResult) + request->Size;

    // overflow check: reject if addition wrapped
    if (resultSize < request->Size) {
      KernelLog::Write(Core::LogLevel::Warning,
        "FAT12: Read dropped: result size overflow (requested %u bytes)",
        request->Size);

      FSABI::FileSystemReadResult errResult { false, FSABI::FileSystemError::IOError, 0 };

      _sendReply(request->ReplyPortID, &errResult, sizeof(errResult));

      return;
    }

    UIntPtr resultAlloc = Memory::ABI::Allocate(resultSize);

    if (resultAlloc == 0) {
      FSABI::FileSystemReadResult errResult { false, FSABI::FileSystemError::IOError, 0 };

      _sendReply(request->ReplyPortID, &errResult, sizeof(errResult));

      return;
    }

    auto* result = reinterpret_cast<FSABI::FileSystemReadResult*>(resultAlloc);

    UInt32 bytesRead = 0;

    bool ok = _volume.ReadFile(
      file->Entry,
      request->Offset,
      request->Size,
      result->Data,
      bytesRead
    );

    if (!ok) {
      Memory::ABI::Free(resultAlloc);

      FSABI::FileSystemReadResult errResult { false, FSABI::FileSystemError::IOError, 0 };

      _sendReply(request->ReplyPortID, &errResult, sizeof(errResult));

      return;
    }

    result->Success   = true;
    result->ErrorCode = FSABI::FileSystemError::None;
    result->BytesRead = bytesRead;

    _sendReply(
      request->ReplyPortID,
      result,
      sizeof(FSABI::FileSystemReadResult) + bytesRead
    );

    Memory::ABI::Free(resultAlloc);
  }

  void FAT12Server::_handleStat(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FSABI::FileSystemStatRequest)) return;

    const auto* request = reinterpret_cast<const FSABI::FileSystemStatRequest*>(
      message->Payload
    );

    ResolvedEntry entry;

    if (!_volume.FindEntry(request->Path, &entry)) {
      FSABI::FileSystemStatResult result { false, FSABI::FileSystemError::FileNotFound, {} };

      _sendReply(request->ReplyPortID, &result, sizeof(result));

      return;
    }

    FSABI::FileSystemFileStat stat;

    stat.Size = entry.IsDirectory ? 0 : entry.FileSize;
    stat.Type = entry.IsDirectory
      ? FSABI::FileSystemEntryType::Directory
      : FSABI::FileSystemEntryType::Regular;

    FSABI::FileSystemStatResult result { true, FSABI::FileSystemError::None, stat };

    _sendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void FAT12Server::_handleReadDirectory(const IPCMessage* message) {
    if (message->PayloadSizeInBytes < sizeof(FSABI::FileSystemReadDirectoryRequest))
      return;

    const auto* request = reinterpret_cast<const FSABI::FileSystemReadDirectoryRequest*>(
      message->Payload
    );

    // allocate result buffer for up to 256 directory entries
    static constexpr UInt32 MaxDirEntries = 256;

    Size resultSize =
      sizeof(FSABI::FileSystemReadDirectoryResult) +
      MaxDirEntries * sizeof(FSABI::FileSystemDirectory);

    UIntPtr resultAlloc = Memory::ABI::Allocate(resultSize);

    if (resultAlloc == 0) {
      FSABI::FileSystemReadDirectoryResult errResult { false, FSABI::FileSystemError::IOError, 0 };

      _sendReply(request->ReplyPortID, &errResult, sizeof(errResult));

      return;
    }

    auto* result = reinterpret_cast<FSABI::FileSystemReadDirectoryResult*>(
      resultAlloc
    );

    UInt32 count = 0;

    bool ok = _volume.ListDirectory(
      request->Path,
      result->Entries,
      MaxDirEntries,
      count
    );

    if (!ok) {
      Memory::ABI::Free(resultAlloc);

      FSABI::FileSystemReadDirectoryResult errResult { false, FSABI::FileSystemError::IOError, 0 };

      _sendReply(request->ReplyPortID, &errResult, sizeof(errResult));

      return;
    }

    result->Success = true;
    result->ErrorCode = FSABI::FileSystemError::None;
    result->EntryCount = count;

    _sendReply(
      request->ReplyPortID,
      result,
      sizeof(FSABI::FileSystemReadDirectoryResult)
      + count * sizeof(FSABI::FileSystemDirectory)
    );

    Memory::ABI::Free(resultAlloc);
  }

  void FAT12Server::_sendReply(
    IPCPortID portID,
    const void* payload,
    Size payloadSize
  ) {
    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(portID, IPCPortRights::Send);

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    Kernel::ABI::IPC::Send(sendHandle, payload, payloadSize);
    Kernel::ABI::IPC::Close(sendHandle);
  }

  void FAT12Server::_sendInvalidOperation(IPCPortID replyPortID) {
    FSABI::FileSystemResult result { false, FSABI::FileSystemError::InvalidOperation };

    _sendReply(replyPortID, &result, sizeof(result));
  }
}
