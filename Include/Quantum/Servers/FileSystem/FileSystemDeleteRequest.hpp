/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemDeleteRequest.hpp
 * @brief Declares @ref @QFSAbi::FileSystemDeleteRequest.
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
   * @brief Request structure for the `Delete` operation.
   */
  struct FileSystemDeleteRequest : public FileSystemRequestWithReply {
    /**
     * @brief Null-terminated path of the file to delete.
     */
    char Path[FileSystemMaxPathLength];
  };
}
