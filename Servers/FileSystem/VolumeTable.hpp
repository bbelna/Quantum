/**
 * @file Servers/FileSystem/VolumeTable.hpp
 * @brief Declares @ref @QFSSrv::VolumeTable and related
 *        structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <FileSystemServerTypes.hpp>

namespace Quantum::Servers::FileSystem {
  /**
   * @brief Maximum number of simultaneously mounted volumes.
   */
  constexpr Size MaxVolumes = 26;

  /**
   * @brief Represents a single mounted volume in the volume table.
   */
  struct Volume {
    /**
     * @brief Null-terminated unique volume ID (e.g., `"FDC0"`, `"STRP"`).
     */
    char VolumeID[FileSystemMaxVolumeIDLength];

    /**
     * @brief Null-terminated human-readable volume label (e.g., `"Quantum"`).
     */
    char Label[FileSystemMaxVolumeLabelLength];

    /**
     * @brief @ref IPCPortID of the file system implementation service.
     */
    IPCPortID FileSystemPortID;

    /**
     * @brief @ref ProcessID of the process that mounted this volume.
     */
    ProcessID OwnerPID;

    /**
     * @brief Whether this slot is actively in use.
     */
    bool Active;

    /**
     * @brief Total volume capacity in bytes.
     */
    UInt32 TotalBytes;

    /**
     * @brief Used space in bytes.
     */
    UInt32 UsedBytes;

    /**
     * @brief Free space in bytes.
     */
    UInt32 FreeBytes;
  };

  /**
   * @brief Fixed-size table mapping volume labels to file system
   *        implementation services.
   */
  class VolumeTable {
    public:
      /**
       * @brief Mounts a volume by associating a volume ID and label with a
       *        file system implementation service.
       * @param volumeID Null-terminated unique volume ID.
       * @param label Null-terminated human-readable volume label.
       * @param portID IPC port ID of the FS implementation service.
       * @param ownerPID Process ID of the process requesting the mount.
       * @return Error code indicating success or the reason for failure.
       */
      FileSystemError Mount(
        const char* volumeID,
        const char* label,
        IPCPortID portID,
        ProcessID ownerPID,
        UInt32 totalBytes = 0,
        UInt32 usedBytes = 0,
        UInt32 freeBytes = 0
      );

      /**
       * @brief Unmounts a volume by volume ID.
       * @param volumeID Null-terminated volume ID to unmount.
       * @return Error code indicating success or the reason for failure.
       */
      FileSystemError Unmount(const char* volumeID);

      /**
       * @brief Resolves a path prefix to a mounted volume.
       * @param prefix Null-terminated path prefix (volume ID or label).
       * @param errorOut If non-null, set to the error code on failure.
       * @return Pointer to the volume if resolved; `nullptr` otherwise.
       *         On ambiguous label match, `*errorOut` is set to
       *         `AmbiguousVolume`.
       */
      const Volume* Resolve(
        const char* prefix,
        FileSystemError* errorOut = nullptr
      ) const;

      /**
       * @brief Finds a mounted volume by volume ID.
       * @param volumeID Null-terminated volume ID to search for.
       * @return Pointer to the volume if found; `nullptr` otherwise.
       */
      const Volume* FindByID(const char* volumeID) const;

      /**
       * @brief Copies the labels of all active volumes into an output array.
       * @param out Destination array of @ref FileSystemVolumeInfo entries.
       * @param maxCount Maximum number of entries to fill.
       * @return Number of active volumes copied.
       */
      Size GetActiveVolumes(FileSystemVolumeInfo* out, Size maxCount) const;

    private:
      /**
       * @brief Array of volume slots.
       */
      Volume _volumes[MaxVolumes] = {};

      /**
       * @brief Number of active volumes.
       */
      Size _count = 0;
  };
}
