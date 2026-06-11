/**
 * @file Servers/Device/DeviceController.hpp
 * @brief Declares @ref @QDvSrv::DeviceController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <DeviceServerTypes.hpp>

#include "DeviceManager.hpp"

namespace Quantum::Servers::Device {
  /**
   * @brief Handles device operations: GetDevicesInCategory and AddDevice.
   */
  class DeviceController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref DeviceController.
       * @param kernel Reference to the @ref KernelClient for IPC
       *                     operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       * @param deviceManager Reference to the @ref DeviceManager for
       *                      device management. Must outlive this controller.
       */
      DeviceController(
        KernelClient& kernel,
        ServerLog& log,
        DeviceManager& deviceManager
      );

      /**
       * @brief Dispatches a device operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Reference to the @ref DeviceManager for device management.
       */
      DeviceManager& _deviceManager;

      /**
       * @brief Handles a @ref DeviceOperation::GetDevicesInCategory request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleGetDevicesInCategory(const IPCMessage* message);

      /**
       * @brief Handles a @ref DeviceOperation::AddDevice request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleAddDevice(const IPCMessage* message);
  };
}
