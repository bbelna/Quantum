/**
 * @file Include/Quantum/Servers/FileSystem/Core/FileSystemOperation.hpp
 * @brief Declares @ref @QFSAbi::FileSystemOperation.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Operations supported by the file system server.
   */
  enum class FileSystemOperation : UInt32 {
    // Volume management (1-9)

    /**
     * @brief Mounts a volume, associating a label with a file system
     *        implementation service.
     */
    Mount = 1,

    /**
     * @brief Unmounts a previously mounted volume by label.
     */
    Unmount = 2,

    /**
     * @brief Lists all currently mounted volumes.
     */
    ListVolumes = 3,

    // File operations (10+)

    /**
     * @brief Opens a file, returning a file handle.
     */
    Open = 10,

    /**
     * @brief Closes a previously opened file handle.
     */
    Close = 11,

    /**
     * @brief Reads data from an open file.
     */
    Read = 12,

    /**
     * @brief Writes data to an open file.
     */
    Write = 13,

    /**
     * @brief Retrieves metadata for a file or directory.
     */
    Stat = 14,

    /**
     * @brief Lists the entries in a directory.
     */
    ReadDirectory = 15,

    /**
     * @brief Deletes a file.
     */
    Delete = 16,

    /**
     * @brief Creates a directory.
     */
    CreateDirectory = 17
  };
}
