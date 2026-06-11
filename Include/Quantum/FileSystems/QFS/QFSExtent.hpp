/**
 * @file Include/Quantum/FileSystems/QFS/QFSExtent.hpp
 * @brief Declares @ref QFSExtent.
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
   * @brief QFS file extent.
   *
   * Maps a contiguous range of logical blocks within a file to contiguous
   * physical blocks on the volume.
   */
  struct QFSExtent {
    /**
     * @brief Starting logical block number within the file.
     */
    UInt32 LogicalBlock;

    /**
     * @brief Starting physical block number on the volume.
     */
    UInt32 PhysicalBlock;

    /**
     * @brief Number of contiguous blocks in this extent.
     */
    UInt32 Length;

    /**
     * @brief Extent flags. See @ref QFSExtentFlag.
     */
    UInt32 Flags;
  } __attribute__((packed));

  static_assert(sizeof(QFSExtent) == 16, "QFSExtent must be 16 bytes");
}
