/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemWriteRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemWriteRequest.
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
   * @brief Request structure for the `Write` operation. The data to write
   *        follows the fixed fields as a flexible array.
   */
  struct FileSystemWriteRequest : public FileSystemRequestWithReply {
    /**
     * @brief Handle to the file to write to.
     */
    FileHandle Handle;

    /**
     * @brief Byte offset within the file to begin writing.
     */
    UInt32 Offset;

    /**
     * @brief Number of bytes in the `Data` array.
     */
    UInt32 DataSize;

    /**
     * @brief Inline data to write (variable length).
     */
    UInt8 Data[];
  };
}
