/**
 * @file Servers/Storage/StorageServer.hpp
 * @brief Declares @ref @QStrSrv::StorageServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StorageServerTypes.hpp>

#include "StorageController.hpp"

namespace Quantum::Servers::Storage {
  /**
   * @brief The storage server: the block-device HAL for QuantumOS.
   *
   * Registers a @ref StorageController for all storage operations. The
   * base @ref Server dispatches incoming IPC messages to the controller
   * based on the operation code.
   */
  class StorageServer : public Server {
    public:
      /**
       * @brief Creates a new @ref StorageServer.
       * @param startupClient Reference to the @ref StartupClient for
       *                      signaling readiness. Must outlive this server
       *                      instance.
       * @param storageController Reference to the @ref StorageController
       *                          for storage operations. Must outlive this
       *                          server instance.
       */
      StorageServer(
        StartupClient& startupClient,
        StorageController& storageController
      );
  };
}
