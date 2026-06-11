/**
 * @file Servers/FileSystem/FileSystemServer.hpp
 * @brief Declares @ref @QFSSrv::FileSystemServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <FileSystemServerTypes.hpp>

#include "FileController.hpp"
#include "VolumeController.hpp"

namespace Quantum::Servers::FileSystem {
  /**
   * @brief The file system server responsible for managing volume mounts
   *        and routing file system operations to implementation services.
   *
   * The server acts as a VFS proxy. It registers a
   * @ref VolumeController for volume management operations and a
   * @ref FileController for file operations. The base @ref Server
   * dispatches incoming IPC messages to the appropriate controller
   * based on the operation code.
   */
  class FileSystemServer : public Server {
    public:
      /**
       * @brief Creates a new @ref FileSystemServer.
       * @param startupClient Reference to the @ref StartupClient for
       *                      signaling readiness. Must outlive this server
       *                      instance.
       * @param volumeController Reference to the @ref VolumeController for
       *                         volume management operations. Must outlive
       *                         this server instance.
       * @param fileController Reference to the @ref FileController for
       *                       file operations. Must outlive this server
       *                       instance.
       */
      FileSystemServer(
        StartupClient& startupClient,
        VolumeController& volumeController,
        FileController& fileController
      );
  };
}
