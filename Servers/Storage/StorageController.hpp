/**
 * @file Servers/Storage/StorageController.hpp
 * @brief Declares @ref @QStrSrv::StorageController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StorageServerTypes.hpp>

#include "StorageDeviceRegistry.hpp"

namespace Quantum::Servers::Storage {
  /**
   * @brief Handles all storage operations: driver registration, device
   *        queries, and I/O forwarding.
   *
   * Maintains the device registry and a handle to the driver reply port.
   * I/O operations (Read, Write, Flush, GetMediaStatus) are forwarded to
   * the appropriate driver process via @ref StorageDevice and the reply
   * is relayed to the client.
   */
  class StorageController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref StorageController.
       * @param kernel Reference to the @ref KernelClient for IPC
       *               operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       * @param registry Reference to the @ref StorageDeviceRegistry.
       *                 Must outlive this controller.
       * @param driverReplyHandle IPC handle for the driver reply port,
       *                          opened with Manage+Receive rights.
       */
      StorageController(
        KernelClient& kernel,
        ServerLog& log,
        StorageDeviceRegistry& registry,
        IPCPortResourceID driverReplyHandle
      );

      /**
       * @brief Dispatches a storage operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Registry of all block devices registered by driver processes.
       */
      StorageDeviceRegistry& _registry;

      /**
       * @brief IPC handle for port `DriverABI::ReplyPortID`, opened with
       *        Manage+Receive rights. Passed to device proxies for I/O
       *        forwarding.
       */
      IPCPortResourceID _driverReplyHandle;

      /**
       * @brief Registers a device with the system Device Server.
       * @param device Pointer to the registered device.
       */
      void _registerWithDeviceServer(const StorageDevice* device);

      void _handleRegisterDriver(const IPCMessage* message);
      void _handleGetDevices(const IPCMessage* message);
      void _handleGetDeviceInfo(const IPCMessage* message);
      void _handleRead(const IPCMessage* message);
      void _handleWrite(const IPCMessage* message);
      void _handleFlush(const IPCMessage* message);
      void _handleGetMediaStatus(const IPCMessage* message);

      /**
       * @brief Sends an error-only `StatusResult` to a client's reply port.
       * @param portID The client's reply port.
       * @param error  The error to report.
       */
      void _sendErrorReply(
        IPCPortID portID,
        StorageOperationErrorCode error
      );
  };
}
