/**
 * @file Servers/Device/DeviceServer.cpp
 * @brief Implements @ref @QDvSrv::DeviceServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "DeviceServer.hpp"

namespace Quantum::Servers::Device {
  DeviceServer::DeviceServer(
    StartupClient& startupClient,
    DeviceController& deviceController
  ) : Server(DevicePortID) {
    RegisterController(DeviceOperation::GetDevicesInCategory, deviceController);
    RegisterController(DeviceOperation::AddDevice, deviceController);

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QDvSrv::DeviceServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QDvSrv::DeviceServer and enters the server loop
 * (@ref @QDvSrv::DeviceServer::Run) to handle device requests.
 */
int Main() {
  KernelClient kernel;
  StartupClient startupClient;
  ServerLog log(kernel);
  DeviceManager deviceManager(kernel, log);

  deviceManager.Discover();

  DeviceController deviceController(kernel, log, deviceManager);

  DeviceServer server(
    startupClient,
    deviceController
  );

  return server.Run();
}
