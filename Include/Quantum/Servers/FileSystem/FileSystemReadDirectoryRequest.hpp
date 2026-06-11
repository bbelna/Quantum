/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemReadDirectoryRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemReadDirectoryRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FileSystemConstants.hpp"
#include "FileSystemRequestWithReply.hpp"
#include "FileSystemResult.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Request structure for the `ReadDirectory` operation.
   */
  struct FileSystemReadDirectoryRequest : public FileSystemRequestWithReply {
    /**
     * @brief Null-terminated directory path (e.g., `"System/Dir"`).
     */
    char Path[FileSystemMaxPathLength];
  };
}
