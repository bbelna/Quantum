/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemReadResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemReadResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemError.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Reply payload for the `Read` operation. The actual data follows
   *        the fixed fields as a flexible array.
   */
  struct FileSystemReadResult {
    /**
     * @brief `true` if the read succeeded; `false` otherwise.
     */
    bool Success;

    /**
     * @brief Error code describing the failure reason, or
     *        `FileSystemError::None` on success.
     */
    FileSystemError ErrorCode;

    /**
     * @brief Number of bytes actually read.
     */
    UInt32 BytesRead;

    /**
     * @brief Inline read data (variable length).
     */
    UInt8 Data[];
  };
}
