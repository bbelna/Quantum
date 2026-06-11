/**
 * @file Include/Quantum/FileSystems/QFS/QFSBlockGroupDescriptor.hpp
 * @brief Declares @ref QFSBlockGroupDescriptor.
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
   * @brief QFS block group descriptor.
   *
   * Stored in the block group descriptor table immediately after the
   * journal. Describes the layout and allocation state of one block group.
   */
  struct QFSBlockGroupDescriptor {
    /**
     * @brief Block number of this group's block allocation bitmap.
     */
    UInt32 BlockBitmapBlock;

    /**
     * @brief Block number of this group's index node allocation bitmap.
     */
    UInt32 IndexNodeBitmapBlock;

    /**
     * @brief First block number of this group's index node table.
     */
    UInt32 IndexNodeTableBlock;

    /**
     * @brief Number of free blocks in this group.
     */
    UInt32 FreeBlockCount;

    /**
     * @brief Number of free index nodes in this group.
     */
    UInt32 FreeIndexNodeCount;

    /**
     * @brief Number of directory index nodes in this group.
     */
    UInt32 DirectoryCount;

    /**
     * @brief Group flags (reserved, must be `0`).
     */
    UInt32 Flags;

    /**
     * @brief CRC32 of this descriptor (with this field set to `0`).
     */
    UInt32 Checksum;

    /**
     * @brief Reserved for future use, must be zeroed.
     */
    UInt8 Reserved[32];
  } __attribute__((packed));

  static_assert(
    sizeof(QFSBlockGroupDescriptor) == 64,
    "QFSBlockGroupDescriptor must be 64 bytes"
  );
}
