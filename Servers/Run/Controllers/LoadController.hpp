/**
 * @file Servers/Run/Controllers/LoadController.hpp
 * @brief Declares @ref @QRunSrv::Controllers::LoadController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Process/ProcessTable.hpp>
#include <RunServerTypes.hpp>

namespace Quantum::Servers::Run::Controllers {
  /**
   * @brief Handles executable loading operations.
   */
  class LoadController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref LoadController.
       * @param kernel
       *   Reference to the @ref KernelClient.
       *   Must outlive this controller.
       * @param log
       *   Reference to the @ref ServerLog.
       *   Must outlive this controller.
       * @param processTable
       *   Reference to the @ref ProcessTable.
       *   Must outlive this controller.
       */
      LoadController(
        KernelClient& kernel,
        ServerLog& log,
        ProcessTable& processTable
      );

    private:
      /**
       * @brief Reference to @ref ProcessTable for process tracking.
       */
      ProcessTable& _processTable;

      /**
       * @brief Handles a @ref RunServerOperation::LoadELF request.
       * @param request The validated @ref LoadELFRequest payload.
       * @param message The raw @ref IPCMessage (for trailing data access).
       */
      void _handleLoadELF(
        const LoadELFRequest& request,
        const IPCMessage* message
      );
  };
}
