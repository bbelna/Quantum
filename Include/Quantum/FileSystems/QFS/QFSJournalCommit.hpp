/**
 * @file Include/Quantum/FileSystems/QFS/QFSJournalCommit.hpp
 * @brief Declares @ref QFSJournalCommit.
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
   * @brief QFS journal commit block.
   *
   * Marks a transaction as fully committed. During recovery, a
   * transaction is only replayed if its commit block is found with a
   * matching sequence number.
   */
  struct QFSJournalCommit {
    /**
     * @brief Magic bytes, must be @ref QFSJournalCommitMagic.
     */
    UInt8 Magic[4];

    /**
     * @brief Transaction sequence number (must match the descriptor).
     */
    UInt32 SequenceNumber;

    /**
     * @brief CRC32 of this commit block.
     */
    UInt32 Checksum;

    /**
     * @brief Reserved for future use, must be zeroed.
     */
    UInt8 Reserved[4084];
  } __attribute__((packed));

  static_assert(
    sizeof(QFSJournalCommit) == 4096,
    "QFSJournalCommit must be 4096 bytes"
  );
}
