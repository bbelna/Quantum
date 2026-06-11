/**
 * @file Servers/FileSystem/FileController.hpp
 * @brief Declares @ref @QFSSrv::FileController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <FileSystemServerTypes.hpp>

#include "FileResource.hpp"
#include "VolumeTable.hpp"

namespace Quantum::Servers::FileSystem {
  /**
   * @brief Handles file operations.
   *
   * Path-based operations parse the volume label from the path and
   * forward the request to the appropriate backend service.
   * Handle-based operations look up the file handle table to determine
   * which backend service owns the file.
   */
  class FileController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref FileController.
       * @param kernel Reference to the @ref KernelClient for IPC
       *                     operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       * @param volumeTable Reference to the @ref VolumeTable for volume
       *                    resolution. Must outlive this controller.
       * @param resourceRepository Reference to the
       *                           @ref FileResourceRepository for file
       *                           resource management. Must outlive this
       *                           controller.
       */
      FileController(
        KernelClient& kernel,
        ServerLog& log,
        VolumeTable& volumeTable,
        FileResourceRepository& resourceRepository
      );

      /**
       * @brief Dispatches a file operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Reference to @ref VolumeTable for volume resolution.
       */
      VolumeTable& _volumeTable;

      /**
       * @brief Reference to @ref FileResourceRepository for file resource
       *        management.
       */
      FileResourceRepository& _resourceRepository;

      /**
       * @brief @ref IPCPortID used for receiving forwarded responses from
       *        backend services.
       */
      static constexpr IPCPortID ForwardReplyPort = 600;

      /**
       * @brief Handles a @ref FileSystemOperation::Open request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleOpen(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::Close request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleClose(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::Read request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleRead(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::Write request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleWrite(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::Stat request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleStat(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::ReadDirectory request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleReadDirectory(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::Delete request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleDelete(const IPCMessage* message);

      /**
       * @brief Handles a @ref FileSystemOperation::CreateDirectory request.
       * @param message The @ref IPCMessage containing the request payload.
       */
      void _handleCreateDirectory(const IPCMessage* message);

      /**
       * @brief Parses a path into a volume label and a volume-relative
       *        path.
       * @param path The path to parse.
       * @param labelOut Buffer to receive the null-terminated volume label.
       * @param relativePathOut Receives a pointer into @p path past the
       *                        `"Label/"` prefix (or to the null terminator
       *                        if no slash was found).
       * @return `true` if the path was parsed successfully; `false` if the
       *         path is malformed.
       */
      static bool _parsePath(
        const char* path,
        char* labelOut,
        const char** relativePathOut
      );

      /**
       * @brief Sends a request to a backend service and waits for a
       *        response.
       * @param servicePortID IPC port of the backend service.
       * @param request The request to forward. Its `ReplyPortID` field
       *                will be overwritten with the server's forward reply
       *                port.
       * @param requestSize Total size of the request in bytes.
       * @return The response message, or `nullptr` on failure. The caller
       *         must free the returned message with @ref Free.
       */
      IPCMessage* _forwardAndReceive(
        IPCPortID servicePortID,
        ABIRequestWithReplyPort<FileSystemOperation>* request,
        Size requestSize
      );

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
