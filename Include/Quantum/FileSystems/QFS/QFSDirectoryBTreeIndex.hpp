/**
 * @file Include/Quantum/FileSystems/QFS/QFSDirectoryBTreeIndex.hpp
 * @brief Declares @ref QFSDirectoryBTreeIndex.
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
   * @brief Internal directory B-tree node entry.
   *
   * Points to a child B-tree block covering name hashes >=
   * @ref NameHash.
   */
  struct QFSDirectoryBTreeIndex {
    /**
     * @brief Lowest name hash covered by the child subtree.
     */
    UInt32 NameHash;

    /**
     * @brief Block number of the child B-tree node.
     */
    UInt32 ChildBlock;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSDirectoryBTreeIndex) == 8,
    "QFSDirectoryBTreeIndex must be 8 bytes"
  );
}
