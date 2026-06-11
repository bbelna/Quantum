/**
 * @file Include/Quantum/Servers/FileSystem/Core/FileSystemResult.hpp
 * @brief Declares @ref @QFSAbi::FileSystemResult,
 *        @ref @QFSAbi::FileSystemRequest, and
 *        @ref @QFSAbi::FileSystemRequestWithReply.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI/ABIRequest.hpp>

#include "FileSystemError.hpp"
#include "FileSystemOperation.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Reply payload for generic file system operations.
   */
  struct FileSystemResult {
    /**
     * @brief `true` if the operation succeeded; `false` otherwise.
     */
    bool Success;

    /**
     * @brief Error code describing the failure reason, or
     *        `FileSystemError::None` on success.
     */
    FileSystemError ErrorCode;
  };
}
