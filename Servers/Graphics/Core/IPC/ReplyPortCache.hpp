/**
 * @file Servers/Graphics/Core/IPC/ReplyPortCache.hpp
 * @brief Declares @ref @QGfxSrv::ReplyPortCache.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

namespace Quantum::Servers::Graphics {
  /**
   * @brief Caches an IPC port handle for sending reply messages to
   *        the same client across multiple requests.
   *
   * On the first call (or when the target port ID changes), the
   * cache opens a new handle via @ref KernelClient::OpenIPCPort.
   * Subsequent calls to the same port reuse the cached handle,
   * avoiding the per-message open/close overhead on hot paths
   * like @ref GraphicsServerOperation::FlushBackBuffer.
   */
  class ReplyPortCache {
    public:
      /**
       * @brief Creates a new @ref ReplyPortCache.
       * @param kernel Reference to the @ref KernelClient for IPC
       *               operations. Must outlive this cache.
       */
      ReplyPortCache(KernelClient& kernel);

      /**
       * @brief Sends a reply payload to a client's reply port,
       *        caching the port handle for reuse.
       * @param replyPortID The client's reply @ref IPCPortID.
       * @param payload Opaque pointer to the payload bytes.
       * @param payloadSizeInBytes Size of the payload.
       */
      void Send(
        UInt16 replyPortID,
        const void* payload,
        Size payloadSizeInBytes
      );

    private:
      KernelClient& _kernel;
      IPCPortResourceID _handle = InvalidIPCPortResourceID;
      UInt16 _portID = 0;
  };
}
