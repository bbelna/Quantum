/**
 * @file Servers/Graphics/Controllers/DrawingController.cpp
 * @brief Implements @ref @QGfxSrv::DrawingController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "DrawingController.hpp"

namespace Quantum::Servers::Graphics {
  DrawingController::DrawingController(
    KernelClient& kernel,
    ServerLog& log,
    Display& display
  ) :
    RequestController(
      kernel,
      log,
      Route<&DrawingController::_handleWriteText>(
        GraphicsServerOperation::WriteText
      ),
      Route<&DrawingController::_handleFillRectangle>(
        GraphicsServerOperation::FillRectangle
      ),
      Route<&DrawingController::_handleBlitBuffer>(
        GraphicsServerOperation::BlitBuffer
      ),
      Route<&DrawingController::_handleXORRectangle>(
        GraphicsServerOperation::XORRectangle
      ),
      Route<&DrawingController::_handleDrawText>(
        GraphicsServerOperation::DrawText
      )
    ),
    _display(display)
  {
  }

  void DrawingController::_handleWriteText(
    const GraphicsWriteTextRequest& request,
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        >= sizeof(GraphicsWriteTextRequest) + 1
    ) {
      _display.driver->WriteText(const_cast<char*>(request.Text));
    }
  }

  void DrawingController::_handleFillRectangle(
    const GraphicsFillRectangleRequest& request
  ) {
    _display.FillRectangle(
      request.X,
      request.Y,
      request.Width,
      request.Height,
      request.Color
    );
  }

  void DrawingController::_handleBlitBuffer(
    const GraphicsBlitBufferRequest& request
  ) {
    _display.BlitBuffer(
      request.X,
      request.Y,
      request.Width,
      request.Height,
      request.TransparentColor,
      request.Pixels
    );
  }

  void DrawingController::_handleXORRectangle(
    const GraphicsXORRectangleRequest& request
  ) {
    _display.XORRectangle(
      request.X,
      request.Y,
      request.Width,
      request.Height,
      request.Color
    );
  }

  void DrawingController::_handleDrawText(
    const GraphicsDrawTextRequest& request,
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        >= sizeof(GraphicsDrawTextRequest) + 1
    ) {
      _display.DrawText(
        request.X,
        request.Y,
        request.Text,
        request.TextLength,
        request.ForegroundColor,
        request.BackgroundColor,
        (request.BackgroundColor >> 24) != 0,
        request.FontWeight > 500
      );
    }
  }
}
