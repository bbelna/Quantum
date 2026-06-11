/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemFileStat.hpp
 * @brief Declares @ref @QFSAbi::FileSystemFileStat.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "FileSystemEntryType.hpp"

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Metadata about a file or directory.
   */
  struct FileSystemFileStat {
    /**
     * @brief Size of the file in bytes (0 for directories).
     */
    UInt32 Size;

    /**
     * @brief Type of the entry.
     */
    FileSystemEntryType Type;
  };
}
