/**
 * @file Drivers/Storage/FDC/FDCServer.hpp
 * @brief Declares @ref @QDrvs::Storage::FDC::FDCServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "FDCStorageController.hpp"

namespace Quantum::Drivers::Storage::FDC {
  /**
   * @brief Driver process for the FDC 82077AA floppy disk controller.
   *
   * At startup the server:
   *   1. Opens an auto-assigned IPC port via the @ref Server base class.
   *   2. Probes the FDC and queries CMOS for installed drives.
   *   3. Sends a `RegisterDriver` message to the Storage server,
   *      listing descriptors for each found drive.
   *   4. Registers the @ref FDCStorageController for all driver
   *      operations.
   *   5. Calls @ref StartupClient::Ready().
   *
   * The base @ref Server dispatches incoming IPC messages to the
   * @ref FDCStorageController, which handles
   * `DriverABI::Operation::Read/Write/Flush/GetMediaStatus` requests
   * forwarded by the Storage server.
   */
  class FDCServer : public Server {
    public:
      /**
       * @brief Creates a new @ref FDCServer.
       * @param startupClient
       *   Reference to the @ref StartupClient.
       *   Must outlive this instance.
       * @param controller
       *   Reference to the @ref FDCStorageController.
       *   Must outlive this instance.
       * @param driver
       *   Reference to the @ref FDCDriver.
       *   Must outlive this instance.
       */
      FDCServer(
        StartupClient& startupClient,
        FDCStorageController& controller,
        FDCDriver& driver
      );
  };
}
