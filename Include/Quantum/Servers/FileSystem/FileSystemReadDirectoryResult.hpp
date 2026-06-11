/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemReadDirectoryResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemReadDirectoryResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemError.hpp"
#include "FileSystemDirectory.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Reply payload for the `ReadDirectory` operation. The entries
   *        follow the fixed fields as a flexible array.
   */
  struct FileSystemReadDirectoryResult {
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
     * @brief Number of entries in the `Entries` array.
     */
    UInt32 EntryCount;

    /**
     * @brief Inline directory entries (variable length).
     */
    FileSystemDirectory Entries[];
  };
}
