/**
 * @file Include/Quantum/Clients/FileSystemClient.hpp
 * @brief Declares @ref @QClients::FileSystemClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Servers/FileSystem/ABI.hpp>

namespace Quantum::Clients {
  namespace {
    using namespace Servers::FileSystem::ABI;
  }

  /**
   * @brief Client-side interface to the file system server.
   *
   * Wraps file system server IPC calls behind a clean API. Each method
   * manages its own IPC handles internally.
   */
  class FileSystemClient {
    public:
      /**
       * @brief Maximum path length constant.
       */
      static constexpr Size MaxPathLength
        = Servers::FileSystem::ABI::FileSystemMaxPathLength;

      /**
       * @brief Maximum volume label length constant.
       */
      static constexpr Size MaxVolumeLabelLength
        = Servers::FileSystem::ABI::FileSystemMaxVolumeLabelLength;

      /**
       * @brief Creates a new @ref FileSystemClient instance.
       */
      FileSystemClient() = default;

      /**
       * @brief Lists all currently mounted volumes.
       * @param outVolumes Array to receive volume information.
       * @param maxCount Maximum number of entries to return.
       * @return Number of volumes returned, or -1 on error.
       */
      Int32 ListVolumes(FileSystemVolumeInfo* outVolumes, UInt32 maxCount);

      /**
       * @brief Retrieves metadata for a file or directory.
       * @param path The full path (volume label + path).
       * @param outStat Pointer to receive metadata.
       * @return `true` on success; `false` if not found or on error.
       */
      bool Stat(const char* path, FileSystemFileStat* outStat);

      /**
       * @brief Opens a file.
       * @param path The full path (volume label + path).
       * @param flags Combination of @ref OpenFlags values.
       * @return A valid file handle on success, or 0 on failure.
       */
      FileHandle Open(const char* path, UInt32 flags);

      /**
       * @brief Closes a previously opened file handle.
       * @param handle The file handle to close.
       * @return `true` on success.
       */
      bool Close(FileHandle handle);

      /**
       * @brief Reads data from an open file.
       * @param handle The file handle.
       * @param buffer Destination buffer.
       * @param size Number of bytes to read.
       * @param offset Byte offset within the file.
       * @return Number of bytes actually read, or -1 on error.
       */
      Int32 Read(
        FileHandle handle,
        void* buffer,
        UInt32 size,
        UInt32 offset
      );

      /**
       * @brief Lists the entries in a directory.
       * @param path The full directory path.
       * @param entries Array to receive directory entries.
       * @param maxEntries Maximum number of entries to return.
       * @return Number of entries read, or -1 on error.
       */
      Int32 ReadDirectory(
        const char* path,
        FileSystemDirectory* entries,
        UInt32 maxEntries
      );

      /**
       * @brief Computes the total size of a directory's contents,
       *        recursively including all subdirectories.
       * @param path The full directory path.
       * @return Total size in bytes, or 0 if the directory is empty or
       *         could not be read.
       */
      UInt32 GetDirectorySize(const char* path);

      /**
       * @brief Creates a directory.
       * @param path The full directory path to create.
       * @return `true` if the directory was created successfully.
       */
      bool CreateDirectory(const char* path);
  };
}
