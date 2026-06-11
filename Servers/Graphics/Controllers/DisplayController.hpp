/**
 * @file Servers/Graphics/Controllers/DisplayController.hpp
 * @brief Declares @ref @QGfxSrv::DisplayController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

#include "../Core/Display/Display.hpp"
#include "../Core/Cursor/Cursor.hpp"
#include "../Core/IPC/ReplyPortCache.hpp"

namespace Quantum::Servers::Graphics {
  /**
   * @brief Handles display management operations: SetMode, GetModeInfo,
   *        GetBackBuffer, ScreenBlit, FlushBackBuffer, BeginBatch, and
   *        EndBatch.
   */
  class DisplayController : public RequestController {
    public:
      DisplayController(
        KernelClient& kernel,
        ServerLog& log,
        Display& display,
        Cursor& cursor
      );

    private:
      Display& _display;
      Cursor& _cursor;
      ReplyPortCache _flushAckCache;

      void _handleSetMode(
        const GraphicsSetModeRequest& request
      );

      void _handleGetModeInfo(
        const GraphicsGetModeInfoRequest& request
      );

      void _handleGetBackBuffer(
        const GraphicsGetBackBufferRequest& request
      );

      void _handleScreenBlit(
        const GraphicsScreenBlitRequest& request
      );

      void _handleFlushBackBuffer(
        const GraphicsFlushBackBufferRequest& request
      );

      void _handleBeginBatch(
        const GraphicsBeginBatchRequest& request
      );

      void _handleEndBatch(
        const GraphicsEndBatchRequest& request
      );
  };
}
