/**
 * @file Include/Quantum/FileSystems/QFS/QFSDirectoryFileType.hpp
 * @brief Declares @ref QFSDirectoryFileType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::QFS {
  /**
   * @brief Cached file type for @ref QFSDirectoryEntry::FileType.
   */
  enum class QFSDirectoryFileType : UInt8 {
    /**
     * @brief Unknown file type.
     */
    Unknown = 0,

    /**
     * @brief Regular file.
     */
    RegularFile = 1,

    /**
     * @brief Directory.
     */
    Directory = 2,

    /**
     * @brief Symbolic link.
     */
    SymbolicLink = 3
  };
}
