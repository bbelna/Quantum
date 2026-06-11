/**
 * @file Drivers/Storage/ATA/ATAServer.hpp
 * @brief Declares @ref @QDrvs::Storage::ATA::ATAServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ATAStorageController.hpp"

namespace Quantum::Drivers::Storage::ATA {
  /**
   * @brief Driver process for both ATA/IDE channels (up to four drives).
   *
   * At startup the server:
   *   1. Opens an auto-assigned IPC port via the @ref Server base class.
   *   2. Probes both ATA channels (primary `0x1F0` and secondary `0x170`).
   *   3. Sends a `RegisterDriver` message to the Storage server for each
   *      discovered drive (ATA hard disks and ATAPI optical drives).
   *   4. Registers the @ref ATAStorageController for all driver operations.
   *   5. Calls @ref StartupClient::Ready().
   *
   * The base @ref Server dispatches incoming IPC messages to the
   * @ref ATAStorageController, which handles `DriverABI` I/O requests.
   */
  class ATAServer : public Server {
    public:
      /**
       * @brief Creates a new @ref ATAServer.
       * @param startupClient
       *   Reference to the @ref StartupClient.
       *   Must outlive this instance.
       * @param controller
       *   Reference to the @ref ATAStorageController.
       *   Must outlive this instance.
       * @param driver
       *   Reference to the @ref ATADriver.
       *   Must outlive this instance.
       */
      ATAServer(
        StartupClient& startupClient,
        ATAStorageController& controller,
        ATADriver& driver
      );
  };
}
