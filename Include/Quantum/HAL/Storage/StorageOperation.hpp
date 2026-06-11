/**
 * @file Include/Quantum/HAL/Storage/StorageOperation.hpp
 * @brief Declates @ref @QHAL::Storage::StorageOperation.
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
   * @brief Operations supported by storage devices.
   */
  enum class StorageOperation : UInt32 {
    /**
     * @brief Returns descriptors for all registered block devices.
     */
    GetDevices = 1,

    /**
     * @brief Returns the descriptor for a single device by ID.
     */
    GetDeviceInfo = 2,

    /**
     * @brief Reads one or more sectors from a block device into a
     *        caller-allocated shared memory buffer.
     */
    Read = 3,

    /**
     * @brief Writes one or more sectors to a block device from a
     *        caller-allocated shared memory buffer.
     */
    Write = 4,

    /**
     * @brief Flushes any write-back cache for a device, ensuring all
     *        pending writes have reached the physical medium.
     */
    Flush = 5,

    /**
     * @brief Queries the media presence state of a removable device (e.g.
     *        floppy, optical). For non-removable devices always returns
     *        `Error::None`.
     */
    GetMediaStatus = 6,

    /**
     * @brief Registers a hardware storage driver with the server. Sent by
     *        driver processes during initialization with the IPC port ID on
     *        which the Storage server can send I/O requests back, plus
     *        descriptors for each device the driver manages.
     * @note Fire-and-forget, no reply is sent. Drivers must not wait for a
     *       response after sending this message.
     */
    RegisterDriver = 7
  };
}
