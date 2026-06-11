/**
 * @file Drivers/Storage/FDC/FDCStorageController.hpp
 * @brief Declares @ref @QDrvs::Storage::FDC::FDCStorageController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FDCDriver.hpp"

namespace Quantum::Drivers::Storage::FDC {
  /**
   * @brief Handles storage I/O requests dispatched by the @ref Server base
   *        class on behalf of the FDC driver.
   *
   * Each incoming @ref DriverABI::Operation is routed here by the server's
   * dispatch table. The controller attaches/detaches shared buffers,
   * delegates the actual hardware I/O to @ref FDCDriver, and sends the
   * result back to the Storage server via @ref SendReply.
   */
  class FDCStorageController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref FDCStorageController.
       * @param kernel
       *   Reference to the @ref KernelClient.
       *   Must outlive this controller.
       * @param log
       *   Reference to the @ref ServerLog for logging.
       *   Must outlive this controller.
       * @param driver
       *   Reference to the @ref FDCDriver hardware driver.
       *   Must outlive this controller.
       */
      FDCStorageController(
        KernelClient& kernel,
        ServerLog& log,
        FDCDriver& driver
      );

      /**
       * @brief Dispatches an incoming IPC message to the appropriate handler
       *        based on the @ref DriverABI::Operation code.
       * @param operation The operation code.
       * @param message The received IPC message.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

      /**
       * @brief Sets the device mapping built during hardware probing.
       * @param deviceCount Number of valid entries in @p driveNumbers.
       * @param driveNumbers Maps zero-based local device index to the actual
       *                     FDC drive number (0 = A, 1 = B).
       */
      void SetDeviceMapping(UInt32 deviceCount, const UInt8* driveNumbers);

    private:
      /**
       * @brief Reference to the FDC hardware driver.
       */
      FDCDriver& _driver;

      /**
       * @brief Number of registered devices.
       */
      UInt32 _deviceCount = 0;

      /**
       * @brief Maps local device index to actual FDC drive number.
       */
      UInt8 _driveNumbers[2] = {};

      /**
       * @brief Handles a `Read` request from the Storage server.
       * @param message The received IPC message.
       */
      void _handleRead(const IPCMessage* message);

      /**
       * @brief Handles a `Write` request from the Storage server.
       * @param message The received IPC message.
       */
      void _handleWrite(const IPCMessage* message);

      /**
       * @brief Handles a `Flush` request from the Storage server.
       * @param message The received IPC message.
       */
      void _handleFlush(const IPCMessage* message);

      /**
       * @brief Handles a `GetMediaStatus` request from the Storage server.
       * @param message The received IPC message.
       */
      void _handleGetMediaStatus(const IPCMessage* message);
  };
}
