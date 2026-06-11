/**
 * @file Include/Quantum/Clients/StreamClient.hpp
 * @brief Declares @ref @QClients::StreamClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/OS.hpp>

namespace Quantum::Clients {
  /**
   * @brief Client-side interface to the stream server.
   *
   * Manages the lifecycle of shared-memory byte streams. The stream server
   * allocates and initializes ring buffers; this client provides a clean API
   * for creating streams and signaling when each side is done.
   *
   * @code
   *   StreamClient streams;
   *   Kernel::Memory::SharedBufferID bufferID;
   *   if (streams.CreateStream(bufferID)) {
   *     // pass bufferID to child, attach locally, read/write ring buffer
   *     streams.CloseReader(bufferID);
   *   }
   * @endcode
   */
  class StreamClient {
    public:
      /**
       * @brief Creates a new @ref StreamClient instance.
       */
      StreamClient() = default;

      /**
       * @brief Creates a new stream backed by a shared-memory ring buffer.
       * @param outBufferID Receives the shared buffer ID on success.
       * @param bufferSize Desired buffer size in bytes, or `0` for the
       *        default size.
       * @return `true` if the stream was created successfully.
       */
      bool CreateStream(
        SharedBufferID& outBufferID,
        UInt32 bufferSize = 0
      );

      /**
       * @brief Notifies the stream server that the writer side has closed.
       * @param bufferID The shared buffer ID of the stream.
       */
      void CloseWriter(SharedBufferID bufferID);

      /**
       * @brief Notifies the stream server that the reader side has closed.
       * @param bufferID The shared buffer ID of the stream.
       */
      void CloseReader(SharedBufferID bufferID);
  };
}
