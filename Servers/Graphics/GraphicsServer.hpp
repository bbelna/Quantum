/**
 * @file Servers/Graphics/GraphicsServer.hpp
 * @brief Declares @ref @QGfxSrv::GraphicsServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

#include "Controllers/DrawingController.hpp"
#include "Controllers/CursorController.hpp"
#include "Controllers/DisplayController.hpp"

namespace Quantum::Servers::Graphics {
  /**
   * @brief The graphics server responsible for managing the display,
   *        cursor compositing, and dispatching drawing operations to
   *        the graphics driver through a system-memory back buffer.
   *
   * The server registers a @ref DrawingController for drawing
   * primitives and batch operations, a @ref CursorController for
   * cursor management, and a @ref DisplayController for display
   * configuration and buffer flushing. The base @ref Server
   * dispatches incoming IPC messages to the appropriate controller
   * based on the operation code.
   */
  class GraphicsServer : public Server {
    public:
      /**
       * @brief Creates a new @ref GraphicsServer.
       * @param startupClient Reference to the @ref StartupClient for
       *                      signaling readiness. Must outlive this
       *                      server instance.
       * @param drawingController Reference to the
       *                          @ref DrawingController for drawing
       *                          operations. Must outlive this server
       *                          instance.
       * @param cursorController Reference to the
       *                         @ref CursorController for cursor
       *                         operations. Must outlive this server
       *                         instance.
       * @param displayController Reference to the
       *                          @ref DisplayController for display
       *                          management operations. Must outlive
       *                          this server instance.
       */
      GraphicsServer(
        StartupClient& startupClient,
        DrawingController& drawingController,
        CursorController& cursorController,
        DisplayController& displayController
      );
  };
}
