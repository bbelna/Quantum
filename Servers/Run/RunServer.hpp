/**
 * @file Servers/Run/RunServer.hpp
 * @brief Declares @ref @QRunSrv::RunServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Controllers/LoadController.hpp"
#include "Controllers/ProcessController.hpp"
#include "RunServerTypes.hpp"

namespace Quantum::Servers::Run {
  /**
   * @brief System server that spawns processes from the file system.
   *
   * Registers a @ref LoadController for binary loading operations and
   * a @ref ProcessController for per-@ref Process metadata operations. The
   * base @ref Server dispatches incoming @ref IPCMessage to the appropriate
   * @ref RequestController based on the operation code.
   */
  class RunServer : public Server {
    public:
      /**
       * @brief Creates a new @ref RunServer.
       * @param startupClient
       *   Reference to a @ref StartupClient.
       *   Must outlive this instance.
       * @param loadController
       *   Reference to the @ref LoadController.
       *   Must outlive this instance.
       * @param processController
       *   Reference to the @ref ProcessController.
       *   Must outlive this instance.
       */
      RunServer(
        StartupClient& startupClient,
        LoadController& loadController,
        ProcessController& processController
      );
  };
}
