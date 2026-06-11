/**
 * @file Servers/Storage/StorageDevice.hpp
 * @brief Declares the storage device proxy class.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StorageServerTypes.hpp>

namespace Quantum::Servers::Storage {
  class StorageDeviceRegistry;

  /**
   * @brief Represents a block device registered by a hardware driver process.
   *
   * Each `StorageDevice` instance is a lightweight proxy that the Storage
   * server creates when a driver process sends a `RegisterDriver` message.
   * The proxy stores:
   *   - The IPC port ID of the driver process that owns the device.
   *   - A zero-based local index that identifies this specific device within
   *     that driver process (matching the position in the driver's
   *     `RegisterDriverRequest::Devices` array).
   *   - A cached `StorageDeviceDescriptor` for fast `GetDevices` / `GetDeviceInfo`
   *     responses without a round-trip to the driver.
   *
   * All I/O operations are forwarded to the driver process via the
   * `DriverABI` IPC protocol. The Storage server's `_driverReplyHandle`
   * (port `DriverABI::ReplyPortID`) is passed to each `Forward*` method so
   * that the server can block until the driver completes the operation.
   */
  class StorageDevice {
    public:
      /**
       * @brief Constructs a device proxy.
       * @param kernel Reference to the kernel client for IPC operations.
       * @param driverPortID IPC port of the owning driver process.
       * @param localIndex   Zero-based index within the driver's device array.
       * @param descriptor   Cached descriptor for this device. The `DeviceID`
       *                     field is updated by `StorageDeviceRegistry::Register`.
       */
      StorageDevice(
        KernelClient& kernel,
        IPCPortID driverPortID,
        UInt32 localIndex,
        const StorageDeviceDescriptor& descriptor
      );

      /**
       * @brief Destructs the device proxy.
       */
      virtual ~StorageDevice() = default;

      /**
       * @brief Returns the cached device descriptor. The `DeviceID` field is
       *        valid after registration with `StorageDeviceRegistry`.
       * @return A copy of the cached descriptor.
       */
      StorageDeviceDescriptor GetDescriptor() const;

      /**
       * @brief Returns the opaque numeric ID assigned by the registry.
       * @return The device ID. Zero before registration.
       */
      UInt32 GetID() const { return _id; }

      // -- I/O forwarding ----------------------------------------------------- //

      /**
       * @brief Forwards a read request to the driver process and blocks until
       *        the driver sends a reply on `replyHandle`.
       * @param lba         First sector to read (logical block address).
       * @param count       Number of consecutive sectors to read.
       * @param bufID       Shared buffer ID the driver will attach, fill, and
       *                    detach.
       * @param replyHandle Handle to port `DriverABI::ReplyPortID`, opened
       *                    at server startup with Manage+Receive rights.
       * @return `StorageOperationErrorCode::None` on success, or an error
       *         from the driver.
       */
      StorageOperationErrorCode ForwardRead(
        UInt64 lba,
        UInt32 count,
        Quantum::Kernel::Memory::SharedBufferID bufID,
        IPCPortResourceID replyHandle
      );

      /**
       * @brief Forwards a write request to the driver process and blocks until
       *        the driver sends a reply.
       * @param lba         First sector to write.
       * @param count       Number of consecutive sectors to write.
       * @param bufID       Shared buffer ID containing the data to write.
       * @param replyHandle Handle to port `DriverABI::ReplyPortID`.
       * @return `StorageOperationErrorCode::None` on success, or an error
       *         from the driver.
       */
      StorageOperationErrorCode ForwardWrite(
        UInt64 lba,
        UInt32 count,
        Quantum::Kernel::Memory::SharedBufferID bufID,
        IPCPortResourceID replyHandle
      );

      /**
       * @brief Forwards a flush request to the driver process and blocks until
       *        the driver sends a reply.
       * @param replyHandle Handle to port `DriverABI::ReplyPortID`.
       * @return `StorageOperationErrorCode::None` on success, or an error
       *         from the driver.
       */
      StorageOperationErrorCode ForwardFlush(
        IPCPortResourceID replyHandle
      );

      /**
       * @brief Forwards a media-status query to the driver process and blocks
       *        until the driver sends a reply.
       * @param replyHandle Handle to port `DriverABI::ReplyPortID`.
       * @return `StorageOperationErrorCode::None` if media is present and
       *         unchanged, or an error code from the driver.
       */
      StorageOperationErrorCode ForwardGetMediaStatus(
        IPCPortResourceID replyHandle
      );

    private:
      /**
       * @brief Reference to the kernel client for IPC operations.
       */
      KernelClient& _kernel;

      /**
       * @brief IPC port ID of the driver process that owns this device.
       */
      IPCPortID _driverPortID;

      /**
       * @brief Zero-based index identifying this device within the driver's
       *        local device array. Embedded in every `DriverABI` request so
       *        the driver knows which hardware unit to address.
       */
      UInt32 _localIndex;

      /**
       * @brief Cached device descriptor. `DeviceID` is filled in by
       *        `StorageDeviceRegistry::Register`.
       */
      StorageDeviceDescriptor _descriptor;

      /**
       * @brief Numeric ID assigned by `StorageDeviceRegistry::Register`.
       *        Zero before registration.
       */
      UInt32 _id = 0;

      bool _sendToDriver(
        IPCPortID driverPort,
        const void* payload,
        Size payloadSize
      );

      /**
       * @brief The registry sets both `_id` and `_descriptor.DeviceID` on
       *        registration; grant access here.
       */
      friend class StorageDeviceRegistry;
  };
}
