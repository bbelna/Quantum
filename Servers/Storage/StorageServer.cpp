/**
 * @file Servers/Storage/StorageServer.cpp
 * @brief Implements @ref @QStrSrv::StorageServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "StorageServer.hpp"

namespace Quantum::Servers::Storage {
  StorageServer::StorageServer(
    StartupClient& startupClient,
    StorageController& storageController
  ) : Server(PortID) {
    RegisterController(StorageOperation::RegisterDriver, storageController);
    RegisterController(StorageOperation::GetDevices, storageController);
    RegisterController(StorageOperation::GetDeviceInfo, storageController);
    RegisterController(StorageOperation::Read, storageController);
    RegisterController(StorageOperation::Write, storageController);
    RegisterController(StorageOperation::Flush, storageController);
    RegisterController(StorageOperation::GetMediaStatus, storageController);

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QStrSrv::StorageServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 */
int Main() {
  KernelClient kernel;
  StartupClient startupClient;
  ServerLog log(kernel);
  StorageDeviceRegistry registry;

  // Open the driver reply port before starting the server so drivers
  // that register early can already reach it.
  IPCPortResourceID driverReplyHandle = kernel.OpenIPCPort(
    DriverABI::ReplyPortID,
    IPCPortRights::Manage | IPCPortRights::Receive
  );

  StorageController storageController(
    kernel,
    log,
    registry,
    driverReplyHandle
  );

  StorageServer server(
    startupClient,
    storageController
  );

  return server.Run();
}
