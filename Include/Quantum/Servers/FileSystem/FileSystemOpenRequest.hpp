/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemOpenRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemOpenRequest.
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
   * @brief Request structure for the `Open` operation.
   */
  struct FileSystemOpenRequest : public FileSystemRequestWithReply {
    /**
     * @brief Combination of `FileSystemOpenFlags` values.
     */
    UInt32 Flags;

    /**
     * @brief Null-terminated path (e.g., `"System/Dir/File"`).
     *        When forwarded to a backend service, the volume label prefix
     *        is stripped.
     */
    char Path[FileSystemMaxPathLength];
  };
}
