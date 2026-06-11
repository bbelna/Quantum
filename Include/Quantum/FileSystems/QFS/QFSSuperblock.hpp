/**
 * @file Include/Quantum/FileSystems/QFS/QFSSuperblock.hpp
 * @brief Declares @ref QFSSuperblock.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "QFSConstants.hpp"
#include "QFSChecksumAlgorithm.hpp"
#include "QFSVolumeState.hpp"

namespace Quantum::FileSystems::QFS {
  /**
   * @brief QFS superblock.
   *
   * Contains all volume-level metadata. A backup copy is stored at block 2.
   * The checksum covers bytes 0..155 with the `Checksum` field treated as
   * zero during computation.
   */
  struct QFSSuperblock {
    /**
     * @brief Magic bytes, must be @ref QFSSuperblockMagic.
     */
    UInt8 Magic[8];

    /**
     * @brief Format version major number.
     */
    UInt16 VersionMajor;

    /**
     * @brief Format version minor number.
     */
    UInt16 VersionMinor;

    /**
     * @brief log2 of the block size in bytes (`12` = 4096).
     */
    UInt32 BlockSizeLog2;

    /**
     * @brief Total number of blocks on the volume.
     */
    UInt32 TotalBlocks;

    /**
     * @brief Number of unallocated blocks.
     */
    UInt32 FreeBlocks;

    /**
     * @brief Total number of index node slots allocated.
     */
    UInt32 TotalIndexNodes;

    /**
     * @brief Number of unallocated index node slots.
     */
    UInt32 FreeIndexNodes;

    /**
     * @brief Index node number of the root directory (always
     *        @ref QFSRootIndexNodeNumber).
     */
    UInt32 RootIndexNodeNumber;

    /**
     * @brief Block number of the first journal block.
     */
    UInt32 JournalStartBlock;

    /**
     * @brief Number of blocks in the journal region.
     */
    UInt32 JournalBlockCount;

    /**
     * @brief Number of block groups on the volume.
     */
    UInt32 BlockGroupCount;

    /**
     * @brief Number of blocks per block group.
     */
    UInt32 BlocksPerGroup;

    /**
     * @brief Number of index node slots per block group.
     */
    UInt32 IndexNodesPerGroup;

    /**
     * @brief Size of one index node record in bytes.
     */
    UInt32 IndexNodeSizeBytes;

    /**
     * @brief First block number available for block group data (after
     *        the journal and block group descriptor table).
     */
    UInt32 FirstDataBlock;

    /**
     * @brief Feature flags. See @ref QFSFeature.
     */
    UInt32 FeatureFlags;

    /**
     * @brief Compatible feature flags. Older implementations that do not
     *        recognize these flags may still mount the volume read-write.
     */
    UInt32 CompatibleFeatureFlags;

    /**
     * @brief Incompatible feature flags. Implementations that do not
     *        recognize these flags must refuse to mount.
     */
    UInt32 IncompatibleFeatureFlags;

    /**
     * @brief Read-only compatible feature flags. Implementations that do
     *        not recognize these flags may mount read-only.
     */
    UInt32 ReadOnlyCompatFlags;

    /**
     * @brief 128-bit volume UUID.
     */
    UInt8 VolumeUUID[16];

    /**
     * @brief Null-terminated UTF-8 volume label.
     */
    char VolumeLabel[QFSMaxVolumeLabelLength];

    /**
     * @brief Number of times this volume has been mounted.
     */
    UInt32 MountCount;

    /**
     * @brief Maximum mount count before filesystem check is suggested.
     *        `0` disables the check.
     */
    UInt32 MaxMountCount;

    /**
     * @brief Volume mount state.
     */
    QFSVolumeState State;

    /**
     * @brief UNIX timestamp of the last mount.
     */
    UInt32 LastMountTime;

    /**
     * @brief UNIX timestamp of the last write operation.
     */
    UInt32 LastWriteTime;

    /**
     * @brief UNIX timestamp of volume creation.
     */
    UInt32 CreationTime;

    /**
     * @brief Checksum algorithm used for metadata integrity.
     */
    QFSChecksumAlgorithm ChecksumType;

    /**
     * @brief CRC32 of this superblock. Computed with this field set to `0`.
     */
    UInt32 Checksum;

    /**
     * @brief Reserved for future use, must be zeroed.
     */
    UInt8 Reserved[3936];
  } __attribute__((packed));

  static_assert(
    sizeof(QFSSuperblock) == 4096, "QFSSuperblock must be 4096 bytes"
  );
}
