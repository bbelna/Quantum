/**
 * @file Servers/Input/InputServer.cpp
 * @brief Implements @ref @QInSrv::InputServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "InputServer.hpp"

namespace Quantum::Servers::Input {
  /// Real-time scheduling priority for the input server (must respond
  /// quickly in the input->overlay->render path).
  static constexpr UInt32 InputServerPriority = 24;

  InputServer::InputServer(
    StartupClient& startupClient,
    InputEventController& inputEventController
  ) : Server(InputPortID) {
    RegisterController(InputOperation::ReportEvent, inputEventController);
    RegisterController(InputOperation::GetNextEvent, inputEventController);
    RegisterController(InputOperation::TryGetNextEvent, inputEventController);

    startupClient.Ready();

    KernelClient kernel;

    kernel.SetThreadPriority(InputServerPriority);
  }
}

/**
 * @brief Entry point for @ref @QInSrv::InputServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QInSrv::InputServer and enters the server loop
 * (@ref @QInSrv::InputServer::Run) to handle input requests.
 */
int Main() {
  KernelClient kernel;
  StartupClient startupClient;
  ServerLog log(kernel);
  InputEventController inputEventController(kernel, log);

  InputServer server(
    startupClient,
    inputEventController
  );

  return server.Run();
}
