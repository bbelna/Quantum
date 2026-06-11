/**
 * @file Include/Quantum/FileSystems/QFS/QFSJournalDescriptor.hpp
 * @brief Declares @ref QFSJournalDescriptor.
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
   * @brief QFS journal descriptor block header.
   *
   * Lists the filesystem blocks journaled in a single transaction.
   * Followed by an array of @ref QFSJournalBlockTag entries.
   */
  struct QFSJournalDescriptor {
    /**
     * @brief Magic bytes, must be @ref QFSJournalDescriptorMagic.
     */
    UInt8 Magic[4];

    /**
     * @brief Transaction sequence number.
     */
    UInt32 SequenceNumber;

    /**
     * @brief Number of block tags in this descriptor.
     */
    UInt32 EntryCount;

    /**
     * @brief CRC32 of this descriptor block.
     */
    UInt32 Checksum;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSJournalDescriptor) == 16,
    "QFSJournalDescriptor must be 16 bytes"
  );
}
