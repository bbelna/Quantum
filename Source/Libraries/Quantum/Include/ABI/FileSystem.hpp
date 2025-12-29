/**
 * @file Libraries/Quantum/Include/ABI/FileSystem.hpp
 * @brief File system syscall wrappers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/IPC.hpp"
#include "ABI/SystemCall.hpp"
#include "Bytes.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * File system syscall wrappers.
   */
  class FileSystem {
    public:
      /**
       * File system operation identifiers.
       */
      enum class Operation : UInt32 {
        /**
         * Lists available volumes.
         */
        ListVolumes = 1,

        /**
         * Retrieves volume info by handle.
         */
        GetVolumeInfo = 2,

        /**
         * Sets a volume label.
         */
        SetVolumeLabel = 3,

        /**
         * Opens a volume by label.
         */
        OpenVolume = 4,

        /**
         * Closes a volume handle.
         */
        CloseVolume = 5,

        /**
         * Opens a file or directory relative to a volume.
         */
        Open = 6,

        /**
         * Closes a file handle.
         */
        Close = 7,

        /**
         * Reads from a file handle.
         */
        Read = 8,

        /**
         * Writes to a file handle.
         */
        Write = 9,

        /**
         * Seeks within a file handle.
         */
        Seek = 10,

        /**
         * Retrieves file info by handle.
         */
        Stat = 11,

        /**
         * Reads a directory entry from a directory handle.
         */
        ReadDirectory = 12,

        /**
         * Creates a directory.
         */
        CreateDirectory = 13,

        /**
         * Creates a file.
         */
        CreateFile = 14,

        /**
         * Removes a file or directory.
         */
        Remove = 15,

        /**
         * Renames a file or directory.
         */
        Rename = 16,

        /**
         * Registers a file system service.
         */
        RegisterService = 17
      };

      /**
       * File system type identifiers.
       */
      enum class Type : UInt32 {
        /**
         * FAT12 file system.
         */
        FAT12 = 1
      };

      /**
       * File handle.
       */
      using Handle = UInt32;

      /**
       * Volume handle.
       */
      using VolumeHandle = UInt32;

      /**
       * Maximum length for volume labels.
       */
      static constexpr UInt32 maxLabelLength = 16;

      /**
       * Maximum length for directory entry names.
       */
      static constexpr UInt32 maxDirectoryLength = 32;

      /**
       * File information descriptor.
       */
      struct FileInfo {
        /**
         * File size in bytes.
         */
        UInt32 sizeBytes;

        /**
         * File attribute flags.
         */
        UInt32 attributes;

        /**
         * FAT create time.
         */
        UInt16 createTime;

        /**
         * FAT create date.
         */
        UInt16 createDate;

        /**
         * FAT last access date.
         */
        UInt16 accessDate;

        /**
         * FAT last write time.
         */
        UInt16 writeTime;

        /**
         * FAT last write date.
         */
        UInt16 writeDate;
      };

      /**
       * Volume information descriptor.
       */
      struct VolumeInfo {
        /**
         * Volume label (null-terminated).
         */
        char label[maxLabelLength];

        /**
         * File system type identifier.
         */
        UInt32 fsType;

        /**
         * Bytes per sector.
         */
        UInt32 sectorSize;

        /**
         * Total sector count.
         */
        UInt32 sectorCount;

        /**
         * Free sector count.
         */
        UInt32 freeSectors;
      };

      /**
       * Directory entry descriptor.
       */
      struct DirectoryEntry {
        /**
         * Entry name (null-terminated).
         */
        char name[maxDirectoryLength];

        /**
         * Entry attribute flags.
         */
        UInt32 attributes;

        /**
         * Entry size in bytes.
         */
        UInt32 sizeBytes;

        /**
         * FAT create time.
         */
        UInt16 createTime;

        /**
         * FAT create date.
         */
        UInt16 createDate;

        /**
         * FAT last access date.
         */
        UInt16 accessDate;

        /**
         * FAT last write time.
         */
        UInt16 writeTime;

        /**
         * FAT last write date.
         */
        UInt16 writeDate;
      };

      /**
       * Volume entry descriptor.
       */
      struct VolumeEntry {
        /**
         * Volume label (null-terminated).
         */
        char label[maxLabelLength];

        /**
         * File system type identifier.
         */
        UInt32 fsType;
      };

      /**
       * IPC message header size for file system service messages.
       */
      static constexpr UInt32 messageHeaderBytes = 7 * sizeof(UInt32);

      /**
       * IPC message data bytes for file system service messages.
       */
      static constexpr UInt32 messageDataBytes
        = IPC::maxPayloadBytes - messageHeaderBytes;

      /**
       * File system service IPC message.
       */
      struct ServiceMessage {
        /**
         * Operation identifier.
         */
        UInt32 op;

        /**
         * Status code (0 success, non-zero failure).
         */
        UInt32 status;

        /**
         * Reply port id for responses.
         */
        UInt32 replyPortId;

        /**
         * First argument.
         */
        UInt32 arg0;

        /**
         * Second argument.
         */
        UInt32 arg1;

        /**
         * Third argument.
         */
        UInt32 arg2;

        /**
         * Payload length in bytes.
         */
        UInt32 dataLength;

        /**
         * Payload data.
         */
        UInt8 data[messageDataBytes];
      };

      /**
       * Lists available volumes.
       * @param outEntries
       *   Output buffer for volume entries.
       * @param maxEntries
       *   Maximum number of entries to write.
       * @return
       *   Number of entries written, or 0 on failure.
       */
      static UInt32 ListVolumes(void* outEntries, UInt32 maxEntries) {
        ServiceMessage request {};
        ServiceMessage response {};
        UInt32 outputBytes = maxEntries
          * static_cast<UInt32>(sizeof(VolumeEntry));

        request.op = static_cast<UInt32>(Operation::ListVolumes);
        request.arg0 = 0;
        request.arg1 = maxEntries;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(request, response, outEntries, outputBytes);
      }

      /**
       * Retrieves volume info by handle.
       * @param volume
       *   Volume handle.
       * @param outInfo
       *   Receives volume info.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 GetVolumeInfo(VolumeHandle volume, VolumeInfo& outInfo) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::GetVolumeInfo);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(
          request,
          response,
          &outInfo,
          static_cast<UInt32>(sizeof(outInfo))
        );
      }

      /**
       * Sets the volume label.
       * @param volume
       *   Volume handle.
       * @param label
       *   Null-terminated label string.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 SetVolumeLabel(VolumeHandle volume, CString label) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::SetVolumeLabel);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = CopyString(label, request.data, messageDataBytes);

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Opens a volume by label.
       * @param label
       *   Volume label string.
       * @return
       *   Volume handle, or 0 on failure.
       */
      static VolumeHandle OpenVolume(CString label) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::OpenVolume);
        request.arg0 = 0;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = CopyString(label, request.data, messageDataBytes);

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Closes a volume handle.
       * @param volume
       *   Volume handle to close.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 CloseVolume(VolumeHandle volume) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::CloseVolume);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Opens a file or directory relative to a volume.
       * @param volume
       *   Volume handle.
       * @param path
       *   Null-terminated path string.
       * @param flags
       *   Open flags.
       * @return
       *   File handle, or 0 on failure.
       */
      static Handle Open(VolumeHandle volume, CString path, UInt32 flags) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Open);
        request.arg0 = volume;
        request.arg1 = flags;
        request.arg2 = 0;
        request.dataLength = CopyString(path, request.data, messageDataBytes);

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Closes a file handle.
       * @param handle
       *   File handle to close.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Close(Handle handle) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Close);
        request.arg0 = handle;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Reads from a file handle.
       * @param handle
       *   File handle.
       * @param buffer
       *   Output buffer.
       * @param length
       *   Number of bytes to read.
       * @return
       *   Number of bytes read, or 0 on failure.
       */
      static UInt32 Read(Handle handle, void* buffer, UInt32 length) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Read);
        request.arg0 = handle;
        request.arg1 = length;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(request, response, buffer, length);
      }

      /**
       * Writes to a file handle.
       * @param handle
       *   File handle.
       * @param buffer
       *   Input buffer.
       * @param length
       *   Number of bytes to write.
       * @return
       *   Number of bytes written, or 0 on failure.
       */
      static UInt32 Write(Handle handle, const void* buffer, UInt32 length) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Write);
        request.arg0 = handle;
        request.arg1 = length;
        request.arg2 = 0;
        request.dataLength = 0;

        if (buffer && length <= messageDataBytes) {
          ::Quantum::CopyBytes(request.data, buffer, length);

          request.dataLength = length;
        }

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Seeks within a file handle.
       * @param handle
       *   File handle.
       * @param offset
       *   Offset in bytes.
       * @param origin
       *   Seek origin (0=begin,1=cur,2=end).
       * @return
       *   New offset, or 0 on failure.
       */
      static UInt32 Seek(Handle handle, UInt32 offset, UInt32 origin) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Seek);
        request.arg0 = handle;
        request.arg1 = offset;
        request.arg2 = origin;
        request.dataLength = 0;

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Retrieves file info by handle.
       * @param handle
       *   File handle.
       * @param outInfo
       *   Receives file info.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Stat(Handle handle, FileInfo& outInfo) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Stat);
        request.arg0 = handle;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(
          request,
          response,
          &outInfo,
          static_cast<UInt32>(sizeof(outInfo))
        );
      }

      /**
       * Reads a directory entry from a directory handle.
       * @param handle
       *   Directory handle.
       * @param outEntry
       *   Receives directory entry data.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 ReadDirectory(Handle handle, DirectoryEntry& outEntry) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::ReadDirectory);
        request.arg0 = handle;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(
          request,
          response,
          &outEntry,
          static_cast<UInt32>(sizeof(outEntry))
        );
      }

      /**
       * Creates a directory.
       * @param volume
       *   Volume handle.
       * @param path
       *   Null-terminated directory path.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 CreateDirectory(VolumeHandle volume, CString path) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::CreateDirectory);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = CopyString(path, request.data, messageDataBytes);

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Creates a file.
       * @param volume
       *   Volume handle.
       * @param path
       *   Null-terminated file path.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 CreateFile(VolumeHandle volume, CString path) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::CreateFile);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = CopyString(path, request.data, messageDataBytes);

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Removes a file or directory.
       * @param volume
       *   Volume handle.
       * @param path
       *   Null-terminated path.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Remove(VolumeHandle volume, CString path) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::Remove);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = CopyString(path, request.data, messageDataBytes);

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Renames a file or directory.
       * @param volume
       *   Volume handle.
       * @param fromPath
       *   Source path.
       * @param toPath
       *   Destination path.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Rename(
        VolumeHandle volume,
        CString fromPath,
        CString toPath
      ) {
        ServiceMessage request {};
        ServiceMessage response {};
        UInt32 offset = 0;

        request.op = static_cast<UInt32>(Operation::Rename);
        request.arg0 = volume;
        request.arg1 = 0;
        request.arg2 = 0;
        request.dataLength = 0;

        offset = CopyString(fromPath, request.data, messageDataBytes);

        if (offset > 0 && offset < messageDataBytes) {
          UInt32 remaining = messageDataBytes - offset;

          request.dataLength = offset
            + CopyString(
              toPath,
              request.data + offset,
              remaining
            );
        }

        return SendRequest(request, response, nullptr, 0);
      }

      /**
       * Registers a file system service with the kernel.
       * @param type
       *   File system type identifier.
       * @param portId
       *   IPC port owned by the service.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 RegisterService(Type type, UInt32 portId) {
        ServiceMessage request {};
        ServiceMessage response {};

        request.op = static_cast<UInt32>(Operation::RegisterService);
        request.arg0 = static_cast<UInt32>(type);
        request.arg1 = portId;
        request.arg2 = 0;
        request.dataLength = 0;

        return SendRequest(request, response, nullptr, 0);
      }

    private:
      static UInt32 CopyString(CString src, UInt8* dest, UInt32 maxBytes) {
        if (!src || !dest || maxBytes == 0) {
          return 0;
        }

        UInt32 length = 0;

        while (length + 1 < maxBytes && src[length] != '\0') {
          dest[length] = static_cast<UInt8>(src[length]);
          ++length;
        }

        dest[length] = '\0';

        return length + 1;
      }

      static UInt32 SendRequest(
        ServiceMessage& request,
        ServiceMessage& response,
        void* output,
        UInt32 outputBytes
      ) {
        UInt32 replyPortId = IPC::CreatePort();

        if (replyPortId == 0) {
          return 0;
        }

        request.replyPortId = replyPortId;

        IPC::Message msg {};
        UInt32 requestBytes = messageHeaderBytes + request.dataLength;

        msg.length = requestBytes;

        ::Quantum::CopyBytes(msg.payload, &request, requestBytes);

        IPC::Handle replyHandle = IPC::OpenPort(
          replyPortId,
          IPC::RightReceive | IPC::RightManage
        );

        if (replyHandle == 0) {
          IPC::DestroyPort(replyPortId);

          return 0;
        }

        IPC::Handle fsHandle = IPC::OpenPort(
          IPC::Ports::FileSystem,
          IPC::RightSend
        );

        if (fsHandle == 0) {
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        if (IPC::Send(fsHandle, msg) != 0) {
          IPC::CloseHandle(fsHandle);
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        IPC::CloseHandle(fsHandle);

        IPC::Message reply {};

        if (IPC::Receive(replyHandle, reply) != 0) {
          IPC::DestroyPort(replyHandle);
          IPC::CloseHandle(replyHandle);

          return 0;
        }

        UInt32 copyBytes = reply.length;

        if (copyBytes > sizeof(response)) {
          copyBytes = sizeof(response);
        }

        ::Quantum::CopyBytes(&response, reply.payload, copyBytes);

        if (output && outputBytes > 0 && response.dataLength > 0) {
          UInt32 responseBytes = response.dataLength;

          if (responseBytes > outputBytes) {
            responseBytes = outputBytes;
          }

          ::Quantum::CopyBytes(output, response.data, responseBytes);
        }

        UInt32 status = response.status;

        IPC::DestroyPort(replyHandle);
        IPC::CloseHandle(replyHandle);

        return status;
      }
  };
}
