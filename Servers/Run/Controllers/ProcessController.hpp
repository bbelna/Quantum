/**
 * @file Servers/Run/Controllers/ProcessController.hpp
 * @brief Declares @ref @QRunSrv::ProcessController.
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
   * @brief Handles per-process metadata operations.
   */
  class ProcessController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref ProcessController.
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
      ProcessController(
        KernelClient& kernel,
        ServerLog& log,
        ProcessTable& processTable
      );

    private:
      /**
       * @brief Reference to the @ref ProcessTable for process metadata.
       */
      ProcessTable& _processTable;

      /**
       * @brief Handles a @ref RunServerOperation::SetWorkingDirectory request.
       * @param request The validated @ref SetWorkingDirectoryRequest payload.
       */
      void _handleSetWorkingDirectory(
        const SetWorkingDirectoryRequest& request
      );

      /**
       * @brief Handles a @ref RunServerOperation::GetWorkingDirectory request.
       * @param request The validated @ref GetWorkingDirectoryRequest payload.
       */
      void _handleGetWorkingDirectory(
        const GetWorkingDirectoryRequest& request
      );

      /**
       * @brief Handles a @ref RunServerOperation::GetProgramDirectory request.
       * @param request The validated @ref GetProgramDirectoryRequest payload.
       */
      void _handleGetProgramDirectory(
        const GetProgramDirectoryRequest& request
      );
  };
}
