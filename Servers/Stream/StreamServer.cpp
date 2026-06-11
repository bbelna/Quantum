/**
 * @file Servers/Stream/StreamServer.cpp
 * @brief Implements @ref @QStrmSrv::StreamServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StreamServer.hpp"

namespace Quantum::Servers::Stream {
  StreamServer::StreamServer(
    StartupClient& startupClient,
    StreamController& streamController
  ) : Server(StreamPortID) {
    RegisterController(StreamOperation::CreateStream, streamController);
    RegisterController(StreamOperation::CloseWriter, streamController);
    RegisterController(StreamOperation::CloseReader, streamController);

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QStrmSrv::StreamServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QStrmSrv::StreamServer and enters the server loop
 * (@ref @QStrmSrv::StreamServer::Run) to handle stream requests.
 */
int Main() {
  KernelClient kernelClient;
  StartupClient startupClient;
  ServerLog serverLog(kernelClient);
  StreamController streamController(
    kernelClient,
    serverLog
  );
  StreamServer streamServer(
    startupClient,
    streamController
  );

  return streamServer.Run();
}
