/**
 * @file Include/Quantum/FileSystems/QFS/QFSExtentTreeHeader.hpp
 * @brief Declares @ref QFSExtentTreeHeader.
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
   * @brief QFS extent tree node header.
   */
  struct QFSExtentTreeHeader {
    /**
     * @brief Tree level. `0` = leaf node (contains @ref QFSExtent
     *        records), `>0` = internal node (contains child pointers).
     */
    UInt16 Level;

    /**
     * @brief Number of entries in this node.
     */
    UInt16 EntryCount;

    /**
     * @brief Maximum number of entries this node can hold.
     */
    UInt16 MaxEntries;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt16 Reserved;

    /**
     * @brief CRC32 of this tree block.
     */
    UInt32 Checksum;

    /**
     * @brief Reserved, must be `0`.
     */
    UInt32 Reserved2;
  } __attribute__((packed));

  static_assert(
    sizeof(QFSExtentTreeHeader) == 16,
    "QFSExtentTreeHeader must be 16 bytes"
  );
}
