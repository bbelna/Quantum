/**
 * @file Include/Quantum/FileSystems/QFS/QFSExtentTreeIndex.hpp
 * @brief Declares @ref QFSExtentTreeIndex.
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
   * @brief Internal QFS extent tree node entry.
   *
   * Points to a child tree block covering logical blocks >=
   * @ref LogicalBlock.
   */
  struct QFSExtentTreeIndex {
    /**
     * @brief Lowest logical block covered by the child subtree.
     */
    UInt32 LogicalBlock;

    /**
     * @brief Block number of the child tree node.
     */
    UInt32 ChildBlock;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSExtentTreeIndex) == 8,
    "QFSExtentTreeIndex must be 8 bytes"
  );
}
