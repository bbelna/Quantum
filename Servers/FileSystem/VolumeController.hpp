/**
 * @file Servers/FileSystem/VolumeController.hpp
 * @brief Declares @ref @QFSSrv::VolumeController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <FileSystemServerTypes.hpp>

#include "VolumeTable.hpp"

namespace Quantum::Servers::FileSystem {
  /**
   * @brief Handles volume management operations: Mount, Unmount, and
   *        ListVolumes.
   */
  class VolumeController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref VolumeController.
       * @param kernel Reference to the @ref KernelClient for IPC
       *                     operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       * @param volumeTable Reference to the @ref VolumeTable for volume
       *                    management. Must outlive this controller.
       */
      VolumeController(
        KernelClient& kernel,
        ServerLog& log,
        VolumeTable& volumeTable
      );

      /**
       * @brief Dispatches a volume operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Reference to @ref VolumeTable for volume management.
       */
      VolumeTable& _volumeTable;

      /**
       * @brief Handles a @ref FileSystemOperation::Mount request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleMount(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::Unmount request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleUnmount(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::ListVolumes request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleListVolumes(const IPCMessage* message);

      /**
       * @brief Sends an error reply to a client's reply port.
       * @param replyPortID The client's reply port ID.
       * @param error The error code to report.
       */
      void _sendErrorReply(
        IPCPortID replyPortID,
        FileSystemError error
      );
  };
}
