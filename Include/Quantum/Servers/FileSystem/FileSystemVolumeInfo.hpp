/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemVolumeInfo.hpp
 * @brief Declares @ref @QFSAbi::FileSystemVolumeInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "FileSystemConstants.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Information about a mounted volume.
   */
  struct FileSystemVolumeInfo {
    /**
     * @brief Null-terminated volume ID.
     */
    char VolumeID[FileSystemMaxVolumeIDLength];

    /**
     * @brief Null-terminated human-readable volume label.
     */
    char Label[FileSystemMaxVolumeLabelLength];

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
