/**
 * @file Include/Quantum/FileSystems/QFS/QFSIndexNodeFlag.hpp
 * @brief Declares @ref QFSIndexNodeFlag.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::QFS {
  /**
   * @brief QFS index node flag bitmask.
   */
  enum class QFSIndexNodeFlag : UInt32 {
    /**
     * @brief Data is stored inline in the
     *        @ref QFSIndexNode::InlineData field.
     */
    InlineData = 1 << 0,

    /**
     * @brief File uses an extent tree (more than
     *        @ref QFSMaxInlineExtents extents).
     */
    ExtentTree = 1 << 1,

    /**
     * @brief Directory uses a B-tree (set for all directories).
     */
    DirectoryBTree = 1 << 2,

    /**
     * @brief File cannot be modified.
     */
    Immutable = 1 << 3,

    /**
     * @brief File can only be appended to.
     */
    AppendOnly = 1 << 4
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::FileSystems::QFS, QFSIndexNodeFlag
)
