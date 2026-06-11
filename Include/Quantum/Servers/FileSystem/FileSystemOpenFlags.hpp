/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemOpenFlags.hpp
 * @brief Declares @ref @QFSAbi::FileSystemOpenFlags.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Core/Enum.hpp>

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Flags for file open operations.
   */
  enum class FileSystemOpenFlags : UInt32 {
    /**
     * @brief Open the file for reading.
     */
    Read = 1u << 0,

    /**
     * @brief Open the file for writing.
     */
    Write = 1u << 1,

    /**
     * @brief Create the file if it does not exist.
     */
    Create = 1u << 2,

    /**
     * @brief Truncate the file to zero length on open.
     */
    Truncate = 1u << 3,

    /**
     * @brief Position writes at the end of the file.
     */
    Append = 1u << 4
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Servers::FileSystem::ABI, FileSystemOpenFlags
)
