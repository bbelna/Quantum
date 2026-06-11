/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemReadRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemReadRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileHandle.hpp"
#include "FileSystemRequestWithReply.hpp"
#include "FileSystemResult.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Request structure for the `Read` operation.
   */
  struct FileSystemReadRequest : public FileSystemRequestWithReply {
    /**
     * @brief Handle to the file to read from.
     */
    FileHandle Handle;

    /**
     * @brief Byte offset within the file to begin reading.
     */
    UInt32 Offset;

    /**
     * @brief Number of bytes to read.
     */
    UInt32 Size;
  };
}
