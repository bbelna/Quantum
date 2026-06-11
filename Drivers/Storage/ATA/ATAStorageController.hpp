/**
 * @file Drivers/Storage/ATA/ATAStorageController.hpp
 * @brief Declares @ref @QDrvs::Storage::ATA::ATAStorageController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ATADriver.hpp"

namespace Quantum::Drivers::Storage::ATA {
  /**
   * @brief Handles storage I/O requests dispatched by the @ref Server base
   *        class on behalf of the ATA driver.
   *
   * Each incoming @ref DriverABI::Operation is routed here by the server's
   * dispatch table. The controller attaches/detaches shared buffers,
   * delegates the actual hardware I/O to @ref ATADriver, and sends the
   * result back to the Storage server via @ref SendReply.
   */
  class ATAStorageController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref ATAStorageController.
       * @param kernel
       *   Reference to the @ref KernelClient.
       *   Must outlive this controller.
       * @param log
       *   Reference to the @ref ServerLog for logging.
       *   Must outlive this controller.
       * @param driver
       *   Reference to the @ref ATADriver hardware driver.
       *   Must outlive this controller.
       */
      ATAStorageController(
        KernelClient& kernel,
        ServerLog& log,
        ATADriver& driver
      );

      /**
       * @brief Dispatches an incoming IPC message to the appropriate handler
       *        based on the @ref DriverABI::Operation code.
       * @param operation The operation code.
       * @param message The received IPC message.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Reference to the ATA hardware driver.
       */
      ATADriver& _driver;

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
