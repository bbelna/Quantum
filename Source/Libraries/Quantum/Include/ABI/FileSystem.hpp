/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/FileSystem.hpp
 * File system syscall wrappers.
 */

#pragma once

#include "ABI/IPC.hpp"
#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI {
  /**
   * File system syscall wrappers.
   */
  class FileSystem {
    public:
      /**
       * Filesystem type identifiers.
       */
      enum class Type : UInt32 {
        /**
         * FAT12 filesystem.
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
         * Filesystem type identifier.
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
      };

      /**
       * IPC message header size for filesystem service messages.
       */
      static constexpr UInt32 messageHeaderBytes = 7 * sizeof(UInt32);

      /**
       * IPC message data bytes for filesystem service messages.
       */
      static constexpr UInt32 messageDataBytes
        = IPC::maxPayloadBytes - messageHeaderBytes;

      /**
       * Filesystem service IPC message.
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
        return InvokeSystemCall(
          SystemCall::FileSystem_ListVolumes,
          reinterpret_cast<UInt32>(outEntries),
          maxEntries,
          0
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_GetVolumeInfo,
          volume,
          reinterpret_cast<UInt32>(&outInfo),
          0
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
        return InvokeSystemCall(
          SystemCall::FileSystem_SetVolumeLabel,
          volume,
          reinterpret_cast<UInt32>(label),
          0
        );
      }

      /**
       * Opens a volume by label.
       * @param label
       *   Volume label string.
       * @return
       *   Volume handle, or 0 on failure.
       */
      static VolumeHandle OpenVolume(CString label) {
        return InvokeSystemCall(
          SystemCall::FileSystem_OpenVolume,
          reinterpret_cast<UInt32>(label),
          0,
          0
        );
      }

      /**
       * Closes a volume handle.
       * @param volume
       *   Volume handle to close.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 CloseVolume(VolumeHandle volume) {
        return InvokeSystemCall(
          SystemCall::FileSystem_CloseVolume,
          volume,
          0,
          0
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Open,
          volume,
          reinterpret_cast<UInt32>(path),
          flags
        );
      }

      /**
       * Closes a file handle.
       * @param handle
       *   File handle to close.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Close(Handle handle) {
        return InvokeSystemCall(
          SystemCall::FileSystem_Close,
          handle,
          0,
          0
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Read,
          handle,
          reinterpret_cast<UInt32>(buffer),
          length
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Write,
          handle,
          reinterpret_cast<UInt32>(buffer),
          length
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Seek,
          handle,
          offset,
          origin
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Stat,
          handle,
          reinterpret_cast<UInt32>(&outInfo),
          0
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
        return InvokeSystemCall(
          SystemCall::FileSystem_ReadDirectory,
          handle,
          reinterpret_cast<UInt32>(&outEntry),
          0
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
        return InvokeSystemCall(
          SystemCall::FileSystem_CreateDirectory,
          volume,
          reinterpret_cast<UInt32>(path),
          0
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_CreateFile,
          volume,
          reinterpret_cast<UInt32>(path),
          0
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Remove,
          volume,
          reinterpret_cast<UInt32>(path),
          0
        );
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
        return InvokeSystemCall(
          SystemCall::FileSystem_Rename,
          volume,
          reinterpret_cast<UInt32>(fromPath),
          reinterpret_cast<UInt32>(toPath)
        );
      }

      /**
       * Registers a filesystem service with the kernel.
       * @param type
       *   Filesystem type identifier.
       * @param portId
       *   IPC port owned by the service.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 RegisterService(Type type, UInt32 portId) {
        return InvokeSystemCall(
          SystemCall::FileSystem_RegisterService,
          static_cast<UInt32>(type),
          portId,
          0
        );
      }
  };
}
