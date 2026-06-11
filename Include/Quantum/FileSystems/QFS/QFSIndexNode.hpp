/**
 * @file Include/Quantum/FileSystems/QFS/QFSIndexNode.hpp
 * @brief Declares @ref QFSIndexNode.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "QFSConstants.hpp"

namespace Quantum::FileSystems::QFS {
  /**
   * @brief QFS index node.
   *
   * Every file, directory, and symbolic link has an index node.
   * Index node numbers are 1-based 32-bit indices. Index node 0 is the
   * null sentinel, index node 1 is reserved for the journal, and index
   * node 2 is the root directory.
   */
  struct QFSIndexNode {
    /**
     * @brief File type (bits 15..12) and permission bits (bits 11..0).
     *        See @ref QFSIndexNodeType.
     */
    UInt16 Mode;

    /**
     * @brief Hard link count. A value of `0` indicates a deleted index
     *        node.
     */
    UInt16 LinkCount;

    /**
     * @brief Owner user ID.
     */
    UInt32 UID;

    /**
     * @brief Owner group ID.
     */
    UInt32 GID;

    /**
     * @brief Index node flags. See @ref QFSIndexNodeFlag.
     */
    UInt32 Flags;

    /**
     * @brief File size in bytes (low 32 bits).
     */
    UInt32 SizeBytes;

    /**
     * @brief File size in bytes (high 32 bits, for future >4GB file
     *        support).
     */
    UInt32 SizeHigh;

    /**
     * @brief Number of 4KB blocks allocated to this index node.
     */
    UInt32 BlockCount;

    /**
     * @brief UNIX timestamp of index node creation.
     */
    UInt32 CreationTime;

    /**
     * @brief UNIX timestamp of last data modification.
     */
    UInt32 ModificationTime;

    /**
     * @brief UNIX timestamp of last access.
     */
    UInt32 AccessTime;

    /**
     * @brief UNIX timestamp of deletion (`0` if not deleted).
     */
    UInt32 DeletionTime;

    /**
     * @brief Index node generation number. Incremented on each reuse of
     *        an index node number. Used for handle validation.
     */
    UInt32 Generation;

    /**
     * @brief Number of extents stored in @ref InlineExtents (0..4).
     */
    UInt32 ExtentCount;

    /**
     * @brief Block number of the extent tree root block. `0` if the file
     *        uses only inline extents.
     */
    UInt32 ExtentTreeRoot;

    /**
     * @brief Inline extent storage (up to @ref QFSMaxInlineExtents
     *        extents).
     */
    UInt8 InlineExtents[QFSMaxInlineExtents * 16];

    /**
     * @brief Block number of the directory B-tree root block (directories
     *        only). `0` for non-directory index nodes.
     */
    UInt32 DirectoryBTreeRoot;

    /**
     * @brief Number of entries in this directory. `0` for non-directory
     *        index nodes.
     */
    UInt32 DirectoryEntryCount;

    /**
     * @brief Inline data storage for small files
     *        (< @ref QFSMaxInlineDataSize bytes). Only valid when
     *        @ref QFSIndexNodeFlag::InlineData is set.
     */
    UInt8 InlineData[QFSMaxInlineDataSize];

    /**
     * @brief Reserved for future use, must be zeroed.
     */
    UInt8 Reserved[20];

    /**
     * @brief Self-referential index node number (for corruption
     *        detection).
     */
    UInt32 IndexNodeNumber;

    /**
     * @brief CRC32 of this index node (with this field set to `0`).
     */
    UInt32 Checksum;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSIndexNode) == 256, "QFSIndexNode must be 256 bytes"
  );
}
