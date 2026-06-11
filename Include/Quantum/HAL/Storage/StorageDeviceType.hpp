/**
 * @file Include/Quantum/HAL/Storage/StorageDeviceType.hpp
 * @brief Declates @ref @QHAL::Storage::StorageDeviceType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::HAL::Storage {
  /**
   * @brief Classifies the physical medium attached to a block device.
   */
  enum class StorageDeviceType : UInt32 {
    /**
     * @brief Unknown or unclassified device type.
     */
    Unknown = 0,

    /**
     * @brief Floppy disk drive (FDC 82077AA / NEC 765-compatible). Uses CHS
     *        geometry internally; the server exposes LBA addressing to callers.
     */
    Floppy = 1,

    /**
     * @brief ATA/IDE hard disk (PIO mode). Addressed by 28-bit LBA.
     */
    HardDisk = 2,

    /**
     * @brief ATAPI optical drive (CD-ROM, DVD). Read-only; 2048-byte sectors.
     */
    OpticalDisk = 3,

    /**
     * @brief Memory-backed virtual disk. Sector size and count are
     *        configurable.
     */
    RAMDisk = 4
  };
}
