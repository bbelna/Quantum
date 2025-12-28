/**
 * @file Services/FileSystems/FAT12/Include/File.hpp
 * @brief FAT12 file system file helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::Services::FileSystems::FAT12 {
  class Volume;

  /**
   * FAT12 file read/write helpers.
   */
  class File {
    public:
      /**
       * Initializes with a volume.
       * @param volume
       *   Volume to access.
       */
      void Initialize(Volume& volume);

      /**
       * Reads file data.
       * @param startCluster
       *   First cluster of the file.
       * @param offset
       *   Byte offset in the file.
       * @param buffer
       *   Output buffer.
       * @param length
       *   Bytes to read.
       * @param outRead
       *   Receives bytes read.
       * @param fileSize
       *   File size in bytes.
       * @return
       *   True on success.
       */
      bool Read(
        UInt32 startCluster,
        UInt32 offset,
        UInt8* buffer,
        UInt32 length,
        UInt32& outRead,
        UInt32 fileSize
      );

      /**
       * Writes file data.
       * @param startCluster
       *   First cluster of the file (may be updated).
       * @param offset
       *   Byte offset in the file.
       * @param buffer
       *   Input buffer.
       * @param length
       *   Bytes to write.
       * @param outWritten
       *   Receives bytes written.
       * @param fileSize
       *   Current file size.
       * @param outSize
       *   Receives updated file size.
       * @return
       *   True on success.
       */
      bool Write(
        UInt32& startCluster,
        UInt32 offset,
        const UInt8* buffer,
        UInt32 length,
        UInt32& outWritten,
        UInt32 fileSize,
        UInt32& outSize
      );

    private:
      /**
       * Associated volume.
       */
      Volume* _volume = nullptr;
  };
}
