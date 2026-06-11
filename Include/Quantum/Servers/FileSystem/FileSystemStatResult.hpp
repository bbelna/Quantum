/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemStatResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemStatResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemError.hpp"
#include "FileSystemFileStat.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Reply payload for the `Stat` operation.
   */
  struct FileSystemStatResult {
    /**
     * @brief `true` if the stat succeeded; `false` otherwise.
     */
    bool Success;

    /**
     * @brief Error code describing the failure reason, or
     *        `FileSystemError::None` on success.
     */
    FileSystemError ErrorCode;

    /**
     * @brief File metadata. Valid only when `Success` is `true`.
     */
    FileSystemFileStat Stat;
  };
}
