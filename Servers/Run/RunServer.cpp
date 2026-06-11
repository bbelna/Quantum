/**
 * @file Servers/Run/RunServer.cpp
 * @brief Implements @ref @QRunSrv::RunServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "RunServer.hpp"

namespace Quantum::Servers::Run {
  RunServer::RunServer(
    StartupClient& startupClient,
    LoadController& loadController,
    ProcessController& processController
  ) : Server(RunServerPortID) {
    RegisterController(
      RunServerOperation::LoadELF,
      loadController
    );

    RegisterController(
      RunServerOperation::SetWorkingDirectory,
      processController
    );
    RegisterController(
      RunServerOperation::GetWorkingDirectory,
      processController
    );
    RegisterController(
      RunServerOperation::GetProgramDirectory,
      processController
    );

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QRunSrv::RunServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QRunSrv::RunServer and enters the server loop
 * (@ref @QRunSrv::RunServer::Run) to handle run requests.
 */
int Main() {
  KernelClient kernelClient;
  StartupClient startupClient;
  ServerLog log(kernelClient);
  ProcessTable processTable;
  LoadController loadController(
    kernelClient,
    log,
    processTable
  );
  ProcessController processController(
    kernelClient,
    log,
    processTable
  );
  RunServer server(
    startupClient,
    loadController,
    processController
  );

  return server.Run();
}
