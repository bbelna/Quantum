/**
 * @file Include/Quantum/FileSystems/QFS/QFSJournalBlockTag.hpp
 * @brief Declares @ref QFSJournalBlockTag.
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
   * @brief QFS journal block tag.
   *
   * Identifies one journaled filesystem block within a transaction.
   */
  struct QFSJournalBlockTag {
    /**
     * @brief The filesystem block number being journaled.
     */
    UInt32 FileSystemBlock;

    /**
     * @brief Tag flags. See @ref QFSJournalBlockTagFlag.
     */
    UInt32 Flags;

    /**
     * @brief CRC32 of the journaled data block.
     */
    UInt32 DataChecksum;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSJournalBlockTag) == 12,
    "QFSJournalBlockTag must be 12 bytes"
  );
}
