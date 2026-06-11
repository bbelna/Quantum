/**
 * @file Servers/Stream/StreamController.hpp
 * @brief Declares @ref @QStrmSrv::StreamController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StreamServerTypes.hpp>

#include "ManagedStream.hpp"

namespace Quantum::Servers::Stream {
  /**
   * @brief Handles stream lifecycle operations.
   *
   * The controller acts as a broker: it creates shared-memory ring buffers,
   * initializes their headers, and tracks which sides have closed. Data
   * flows directly between writer and reader through the shared buffer,
   * never through the server.
   */
  class StreamController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref StreamController.
       * @param kernel Reference to the @ref KernelClient for IPC and
       *               memory operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       */
      StreamController(
        KernelClient& kernel,
        ServerLog& log
      );

      /**
       * @brief Dispatches a stream operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

    private:
      /**
       * @brief Active stream slots.
       */
      ManagedStream _streams[MaxStreams] = {};

      /**
       * @brief Handles a @ref StreamOperation::CreateStream request.
       * @param message Pointer to the @ref IPCMessage containing the request.
       */
      void _handleCreateStream(const IPCMessage* message);

      /**
       * @brief Handles a @ref StreamOperation::CloseWriter request.
       * @param message Pointer to the @ref IPCMessage containing the request.
       */
      void _handleCloseWriter(const IPCMessage* message);

      /**
       * @brief Handles a @ref StreamOperation::CloseReader request.
       * @param message Pointer to the @ref IPCMessage containing the request.
       */
      void _handleCloseReader(const IPCMessage* message);

      /**
       * @brief Finds a stream slot by its @ref SharedBufferID.
       * @param bufferID The @ref SharedBufferID to search for.
       * @return Pointer to the slot (@ref ManagedStream), or `nullptr` if not
       *         found.
       */
      ManagedStream* _findStream(SharedBufferID bufferID);

      /**
       * @brief Finds an unused stream slot.
       * @return Pointer to a free slot (@ref ManagedStream), or `nullptr` if
       *         all are in use.
       */
      ManagedStream* _findFreeSlot();

      /**
       * @brief Releases a stream slot if both sides have closed.
       * @param stream Pointer to the @ref ManagedStream to check and
       *               potentially release.
       */
      void _tryCleanup(ManagedStream* stream);
  };
}
