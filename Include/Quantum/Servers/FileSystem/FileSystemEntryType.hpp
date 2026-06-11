/**
 * @file Include/Quantum/Servers/FileSystem/FileSystemEntryType.hpp
 * @brief Declares @ref @QFSAbi::FileSystemEntryType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::FileSystem::ABI {
  /**
   * @brief Describes the type of a file system entry.
   */
  enum class FileSystemEntryType : UInt32 {
    /**
     * @brief A regular file.
     */
    Regular = 0,

    /**
     * @brief A directory.
     */
    Directory = 1
  };
}
