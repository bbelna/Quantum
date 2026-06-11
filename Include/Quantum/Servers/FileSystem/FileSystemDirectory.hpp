/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemDirectory.hpp
 * @brief Declares @ref @QFSAbi::FileSystemDirectory.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "FileSystemConstants.hpp"
#include "FileSystemEntryType.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief A single entry in a directory listing.
   */
  struct FileSystemDirectory {
    /**
     * @brief Null-terminated name of the entry.
     */
    char Name[FileSystemMaxFileNameLength];

    /**
     * @brief Type of the entry.
     */
    FileSystemEntryType Type;

    /**
     * @brief Size of the entry in bytes (0 for directories).
     */
    UInt32 Size;
  };
}
