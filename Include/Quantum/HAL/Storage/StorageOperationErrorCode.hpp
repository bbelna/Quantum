/**
 * @file Include/Quantum/HAL/Storage/StorageOperationErrorCode.hpp
 * @brief Declates @ref @QHAL::Storage::StorageOperationErrorCode.
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
   * @brief Error codes returned by storage server operations.
   */
  enum class StorageOperationErrorCode : UInt32 {
    /**
     * @brief The operation completed successfully.
     */
    None = 0,

    /**
     * @brief The specified device ID does not correspond to any registered
     *        device.
     */
    InvalidDevice = 1,

    /**
     * @brief The requested LBA is beyond the end of the device.
     */
    InvalidLBA = 2,

    /**
     * @brief `SectorCount` is zero or would cause the transfer to exceed the
     *        end of the device.
     */
    InvalidCount = 3,

    /**
     * @brief The hardware reported a read failure.
     */
    ReadError = 4,

    /**
     * @brief The hardware reported a write failure.
     */
    WriteError = 5,

    /**
     * @brief The device exists but is not ready (e.g. floppy motor not yet
     *        at speed, or drive reset in progress).
     */
    DeviceNotReady = 6,

    /**
     * @brief A write was attempted on a read-only or write-protected device.
     */
    WriteProtected = 7,

    /**
     * @brief The hardware did not respond within the expected time.
     */
    Timeout = 8,

    /**
     * @brief Removable media was changed since the last access. The caller
     *        should re-read the device descriptor and restart the operation.
     */
    MediaChanged = 9,

    /**
     * @brief A removable device has no media inserted.
     */
    NoMedia = 10
  };
}
