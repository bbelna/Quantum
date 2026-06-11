/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemOpenResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemOpenResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileHandle.hpp"
#include "FileSystemError.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Reply payload for the `Open` operation.
   */
  struct FileSystemOpenResult {
    /**
     * @brief `true` if the file was opened successfully; `false` otherwise.
     */
    bool Success;

    /**
     * @brief Error code describing the failure reason, or
     *        `FileSystemError::None` on success.
     */
    FileSystemError ErrorCode;

    /**
     * @brief Handle to the opened file. Valid only when `Success` is `true`.
     */
    FileHandle Handle;
  };
}
