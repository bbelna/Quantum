/**
 * @file Include/Quantum/FileSystems/QFS/QFSJournalSuperblock.hpp
 * @brief Declares @ref QFSJournalSuperblock.
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
   * @brief QFS journal superblock (4096 bytes, first block of the journal
   *        region).
   */
  struct QFSJournalSuperblock {
    /**
     * @brief Magic bytes, must be @ref QFSJournalMagic.
     */
    UInt8 Magic[4];

    /**
     * @brief Current transaction sequence number.
     */
    UInt32 SequenceNumber;

    /**
     * @brief Journal-relative block offset of the first active
     *        transaction.
     */
    UInt32 HeadBlock;

    /**
     * @brief Journal-relative block offset past the last active
     *        transaction.
     */
    UInt32 TailBlock;

    /**
     * @brief Total number of blocks in the journal (including this
     *        superblock).
     */
    UInt32 BlockCount;

    /**
     * @brief Block size in bytes (must match the filesystem block size).
     */
    UInt32 BlockSize;

    /**
     * @brief Maximum number of blocks per transaction.
     */
    UInt32 MaxTransactionBlocks;

    /**
     * @brief CRC32 of this journal superblock.
     */
    UInt32 Checksum;

    /**
     * @brief Reserved for future use, must be zeroed.
     */
    UInt8 Reserved[4064];
  } __attribute__((packed));

  static_assert(
    sizeof(QFSJournalSuperblock) == 4096,
    "QFSJournalSuperblock must be 4096 bytes"
  );
}
