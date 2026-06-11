/**
 * @file Include/Quantum/FileSystems/QFS/QFSDirectoryEntry.hpp
 * @brief Declares @ref QFSDirectoryEntry.
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
   * @brief QFS directory entry.
   *
   * Stored in leaf nodes of the directory B-tree, sorted by
   * @ref NameHash. Name bytes follow this header immediately and are
   * NOT null-terminated on disk.
   */
  struct QFSDirectoryEntry {
    /**
     * @brief Index node number of the referenced file or directory.
     */
    UInt32 IndexNodeNumber;

    /**
     * @brief FNV-1a 32-bit hash of the entry name (lowercased for
     *        case-insensitive lookup).
     */
    UInt32 NameHash;

    /**
     * @brief Total size of this entry in bytes (header + name, padded
     *        to 4-byte alignment).
     */
    UInt16 RecordLength;

    /**
     * @brief Length of the name in bytes (not including any padding).
     */
    UInt8 NameLength;

    /**
     * @brief Cached file type from the referenced index node's Mode.
     *        See @ref QFSDirectoryFileType.
     */
    UInt8 FileType;

    // Name bytes follow (NOT null-terminated on disk)
  } __attribute__((packed));

  static_assert(
    sizeof(QFSDirectoryEntry) == 12,
    "QFSDirectoryEntry header must be 12 bytes"
  );
}
