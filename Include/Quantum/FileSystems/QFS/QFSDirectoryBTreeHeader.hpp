/**
 * @file Include/Quantum/FileSystems/QFS/QFSDirectoryBTreeHeader.hpp
 * @brief Declares @ref QFSDirectoryBTreeHeader.
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
   * @brief QFS directory B-tree node header.
   */
  struct QFSDirectoryBTreeHeader {
    /**
     * @brief Tree level. `0` = leaf node (contains directory entries),
     *        `>0` = internal node (contains child pointers).
     */
    UInt16 Level;

    /**
     * @brief Number of entries in this node.
     */
    UInt16 EntryCount;

    /**
     * @brief Free bytes remaining in this node (leaf nodes only).
     */
    UInt16 FreeBytes;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt16 Reserved;

    /**
     * @brief CRC32 of this B-tree block.
     */
    UInt32 Checksum;

    /**
     * @brief Block number of the next leaf node in sequence (for
     *        efficient directory iteration). `0` if this is the last
     *        leaf.
     */
    UInt32 NextLeaf;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSDirectoryBTreeHeader) == 16,
    "QFSDirectoryBTreeHeader must be 16 bytes"
  );
}
