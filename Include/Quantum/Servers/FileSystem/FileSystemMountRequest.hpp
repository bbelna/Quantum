/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemMountRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemMountRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemConstants.hpp"
#include "FileSystemRequestWithReply.hpp"
#include "FileSystemResult.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Request structure for the `Mount` operation.
   */
  struct FileSystemMountRequest : public FileSystemRequestWithReply {
    /**
     * @brief Null-terminated volume ID (e.g., `"FDC0"`, `"STRP"`). Must be
     *        unique across all mounted volumes.
     */
    char VolumeID[FileSystemMaxVolumeIDLength];

    /**
     * @brief Null-terminated human-readable volume label (e.g., `"Quantum"`).
     *        Used for display and user-facing path resolution. Need not be
     *        unique.
     */
    char Label[FileSystemMaxVolumeLabelLength];

    /**
     * @brief IPC port ID of the file system implementation service that
     *        handles operations on this volume.
     */
    IPCPortID ServicePortID;

    /**
     * @brief Total volume capacity in bytes, or 0 if unknown.
     */
    UInt32 TotalBytes;

    /**
     * @brief Used space in bytes, or 0 if unknown.
     */
    UInt32 UsedBytes;

    /**
     * @brief Free space in bytes, or 0 if unknown.
     */
    UInt32 FreeBytes;
  };
}
