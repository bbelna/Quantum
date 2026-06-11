/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemWriteResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemWriteResult.
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
   * @brief Reply payload for the `Write` operation.
   */
  struct FileSystemWriteResult {
    /**
     * @brief `true` if the write succeeded; `false` otherwise.
     */
    bool Success;

    /**
     * @brief Error code describing the failure reason, or
     *        `FileSystemError::None` on success.
     */
    FileSystemError ErrorCode;

    /**
     * @brief Number of bytes actually written.
     */
    UInt32 BytesWritten;
  };
}
