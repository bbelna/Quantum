/**
 * @file Include/Quantum/FileSystems/QFS/Constants.hpp
 * @brief QFS format constants: magic bytes, version, defaults, limits, and
 *        reserved index node numbers.
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
   * @brief Superblock magic bytes: `"QFS1\0\0\0\0"`.
   */
  inline constexpr UInt8 QFSSuperblockMagic[8] = {
    'Q', 'F', 'S', '1', '\0', '\0', '\0', '\0'
  };

  /**
   * @brief Journal superblock magic bytes: `"QJNL"`.
   */
  inline constexpr UInt8 QFSJournalMagic[4] = { 'Q', 'J', 'N', 'L' };

  /**
   * @brief Journal descriptor block magic bytes: `"QDSC"`.
   */
  inline constexpr UInt8 QFSJournalDescriptorMagic[4] = {
    'Q', 'D', 'S', 'C'
  };

  /**
   * @brief Journal commit block magic bytes: `"QCMT"`.
   */
  inline constexpr UInt8 QFSJournalCommitMagic[4] = { 'Q', 'C', 'M', 'T' };

  /**
   * @brief Current QFS format version (major).
   */
  inline constexpr UInt16 QFSFormatVersionMajor = 1;

  /**
   * @brief Current QFS format version (minor).
   */
  inline constexpr UInt16 QFSFormatVersionMinor = 0;

  /**
   * @brief Default block size in bytes.
   */
  inline constexpr UInt32 QFSDefaultBlockSize = 4096;

  /**
   * @brief Default log2(block size).
   */
  inline constexpr UInt32 QFSDefaultBlockSizeLog2 = 12;

  /**
   * @brief Default blocks per block group.
   */
  inline constexpr UInt32 QFSDefaultBlocksPerGroup = 8192;

  /**
   * @brief Default index nodes per block group.
   */
  inline constexpr UInt32 QFSDefaultIndexNodesPerGroup = 1024;

  /**
   * @brief Default index node size in bytes.
   */
  inline constexpr UInt32 QFSDefaultIndexNodeSize = 256;

  /**
   * @brief Maximum filename length in bytes (UTF-8).
   */
  inline constexpr UInt32 QFSMaxFilenameLength = 255;

  /**
   * @brief Maximum volume label length, including null terminator.
   */
  inline constexpr UInt32 QFSMaxVolumeLabelLength = 32;

  /**
   * @brief Maximum number of inline extents per index node.
   */
  inline constexpr UInt32 QFSMaxInlineExtents = 4;

  /**
   * @brief Maximum inline data size in an index node.
   */
  inline constexpr UInt32 QFSMaxInlineDataSize = 100;

  /**
   * @brief Null index node number (unused).
   */
  inline constexpr UInt32 QFSNullIndexNodeNumber = 0;

  /**
   * @brief Journal index node number (reserved).
   */
  inline constexpr UInt32 QFSJournalIndexNodeNumber = 1;

  /**
   * @brief Root directory index node number.
   */
  inline constexpr UInt32 QFSRootIndexNodeNumber = 2;

  /**
   * @brief First user-allocatable index node number.
   */
  inline constexpr UInt32 QFSFirstUserIndexNode = 3;
}
