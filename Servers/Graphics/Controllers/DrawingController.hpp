/**
 * @file Servers/Graphics/Controllers/DrawingController.hpp
 * @brief Declares @ref @QGfxSrv::DrawingController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

#include "../Core/Display/Display.hpp"

namespace Quantum::Servers::Graphics {
  /**
   * @brief Handles drawing operations: WriteText, FillRectangle,
   *        BlitBuffer, XORRectangle, and DrawText.
   */
  class DrawingController : public RequestController {
    public:
      DrawingController(
        KernelClient& kernel,
        ServerLog& log,
        Display& display
      );

    private:
      Display& _display;

      void _handleWriteText(
        const GraphicsWriteTextRequest& request,
        const IPCMessage* message
      );

      void _handleFillRectangle(
        const GraphicsFillRectangleRequest& request
      );

      void _handleBlitBuffer(
        const GraphicsBlitBufferRequest& request
      );

      void _handleXORRectangle(
        const GraphicsXORRectangleRequest& request
      );

      void _handleDrawText(
        const GraphicsDrawTextRequest& request,
        const IPCMessage* message
      );
  };
}
