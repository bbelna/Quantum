/**
 * @file Servers/FileSystem/FileSystemServer.cpp
 * @brief Implements @ref @QFSSrv::FileSystemServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FileSystemServer.hpp"

namespace Quantum::Servers::FileSystem {
  FileSystemServer::FileSystemServer(
    StartupClient& startupClient,
    VolumeController& volumeController,
    FileController& fileController
  ) : Server(FileSystemPortID) {
    RegisterController(FileSystemOperation::Mount, volumeController);
    RegisterController(FileSystemOperation::Unmount, volumeController);
    RegisterController(FileSystemOperation::ListVolumes, volumeController);

    RegisterController(FileSystemOperation::Open, fileController);
    RegisterController(FileSystemOperation::Close, fileController);
    RegisterController(FileSystemOperation::Read, fileController);
    RegisterController(FileSystemOperation::Write, fileController);
    RegisterController(FileSystemOperation::Stat, fileController);
    RegisterController(FileSystemOperation::ReadDirectory, fileController);
    RegisterController(FileSystemOperation::Delete, fileController);
    RegisterController(FileSystemOperation::CreateDirectory, fileController);

    startupClient.Ready();
  }
}

/**
 * @brief Entry point for @ref @QFSSrv::FileSystemServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 * @note This is a system server and should not exit under normal operation.
 *       Returning from this function indicates a failure.
 *
 * Initializes @ref @QFSSrv::FileSystemServer and enters the server loop
 * (@ref @QFSSrv::FileSystemServer::Run) to handle file system requests.
 */
int Main() {
  KernelClient kernel;
  StartupClient startupClient;
  ServerLog log(kernel);
  VolumeTable volumeTable;
  FileResourceRepository resourceRepository;
  VolumeController volumeController(kernel, log, volumeTable);
  FileController fileController(kernel, log, volumeTable, resourceRepository);

  FileSystemServer server(
    startupClient,
    volumeController,
    fileController
  );

  return server.Run();
}
