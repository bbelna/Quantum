/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemListVolumesResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemListVolumesResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemError.hpp"
#include "FileSystemVolumeInfo.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Reply payload for the `ListVolumes` operation. The volume
   *        entries follow the fixed fields as a flexible array.
   */
  struct FileSystemListVolumesResult {
    /**
     * @brief `true` if the listing succeeded; `false` otherwise.
     */
    bool Success;

    /**
     * @brief Error code describing the failure reason, or
     *        `FileSystemError::None` on success.
     */
    FileSystemError ErrorCode;

    /**
     * @brief Number of volumes in the `Volumes` array.
     */
    UInt32 VolumeCount;

    /**
     * @brief Inline volume entries (variable length).
     */
    FileSystemVolumeInfo Volumes[];
  };
}
