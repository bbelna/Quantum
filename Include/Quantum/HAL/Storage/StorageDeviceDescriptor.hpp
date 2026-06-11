/**
 * @file Include/Quantum/HAL/Storage/StorageDeviceDescriptor.hpp
 * @brief Declares @ref @QHAL::Storage::StorageDeviceDescriptor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "StorageDeviceType.hpp"

namespace Quantum::HAL::Storage {
  /**
   * @brief Describes a registered block device.
   *
   * Returned inline in `GetDevicesResult` and `GetDeviceInfoResult`. All
   * string fields are null-terminated and space-padded to their array bounds.
   */
  struct StorageDeviceDescriptor {
    /**
     * @brief Opaque numeric identifier assigned by the storage server.
     *        Pass this value in operation requests to address this device.
     */
    UInt32 DeviceID;

    /**
     * @brief Physical medium classification.
     */
    StorageDeviceType Type;

    /**
     * @brief Short programmatic name (e.g. `"FDC0"`, `"ATA0P0"`).
     */
    char Name[32];

    /**
     * @brief Human-readable display name (e.g. `"Floppy Disk A"`,
     *        `"Primary Master"`).
     */
    char DisplayName[48];

    /**
     * @brief Total number of addressable sectors (LBA 0 through
     *        `SectorCount - 1`).
     */
    UInt64 SectorCount;

    /**
     * @brief Size of one sector in bytes (typically 512 for floppy and HDD,
     *        2048 for optical).
     */
    UInt32 SectorSize;

    /**
     * @brief `true` if the device does not support write operations (e.g.
     *        a CD-ROM or write-protected floppy).
     */
    bool ReadOnly;

    /**
     * @brief `true` if the medium can be removed while the system is running
     *        (floppy, optical). When `true`, callers should use
     *        `GetMediaStatus` to confirm media presence before I/O.
     */
    bool Removable;
  };
}
