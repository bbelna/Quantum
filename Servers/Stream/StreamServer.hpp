/**
 * @file Servers/Stream/StreamServer.hpp
 * @brief Declares @ref @QStrmSrv::StreamServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <StreamServerTypes.hpp>

#include "StreamController.hpp"

namespace Quantum::Servers::Stream {
  /**
   * @brief Manages byte stream lifecycles.
   *
   * The stream server acts as a broker: it creates shared-memory ring
   * buffers, initializes their headers, and tracks which sides have closed.
   * Data flows directly between writer and reader through the shared buffer,
   * never through the server.
   *
   * Registers a @ref StreamController for all stream operations and
   * delegates dispatch to the @ref Server base class.
   */
  class StreamServer : public Server {
    public:
      /**
       * @brief Creates a new @ref StreamServer.
       * @param startupClient
       *   Reference to @ref StartupClient.
       *   Must outlive this instance.
       * @param streamController
       *   Reference to the @ref StreamController.
       *   Must outlive this instance.
       */
      StreamServer(
        StartupClient& startupClient,
        StreamController& streamController
      );
  };
}
