/**
 * @file Servers/Graphics/Controllers/DisplayController.cpp
 * @brief Implements @ref @QGfxSrv::DisplayController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "DisplayController.hpp"

namespace Quantum::Servers::Graphics {
  DisplayController::DisplayController(
    KernelClient& kernel,
    ServerLog& log,
    Display& display,
    Cursor& cursor
  ) :
    RequestController(
      kernel,
      log,
      Route<&DisplayController::_handleSetMode>(
        GraphicsServerOperation::SetMode
      ),
      Route<&DisplayController::_handleGetModeInfo>(
        GraphicsServerOperation::GetModeInfo
      ),
      Route<&DisplayController::_handleGetBackBuffer>(
        GraphicsServerOperation::GetBackBuffer
      ),
      Route<&DisplayController::_handleScreenBlit>(
        GraphicsServerOperation::ScreenBlit
      ),
      Route<&DisplayController::_handleFlushBackBuffer>(
        GraphicsServerOperation::FlushBackBuffer
      ),
      Route<&DisplayController::_handleBeginBatch>(
        GraphicsServerOperation::BeginBatch
      ),
      Route<&DisplayController::_handleEndBatch>(
        GraphicsServerOperation::EndBatch
      )
    ),
    _display(display),
    _cursor(cursor),
    _flushAckCache(kernel)
  {
  }

  void DisplayController::_handleSetMode(
    const GraphicsSetModeRequest& request
  ) {
    _display.driver->SetMode(request.Mode);

    auto info = _display.driver->QueryModeInfo();

    _display.screenWidth = info.Width;
    _display.screenHeight = info.Height;
    _display.bitsPerPixel = info.BitsPerPixel;
    _display.pitch = info.Pitch;

    _display.backBuffer.Allocate(
      _display.screenWidth,
      _display.screenHeight,
      _display.bitsPerPixel
    );

    _display.framebufferBufferID
      = _display.driver->GetFramebufferBufferID();
    _display.hasDirectFramebuffer
      = _display.framebufferBufferID != 0
     && _display.hasHardwareCursor;
  }

  void DisplayController::_handleGetModeInfo(
    const GraphicsGetModeInfoRequest& request
  ) {
    GraphicsGetModeInfoResult result;

    result.Width = _display.screenWidth;
    result.Height = _display.screenHeight;
    result.BitsPerPixel = _display.bitsPerPixel;
    result.Pitch = _display.pitch;
    result.HardwareCursor = _display.hasHardwareCursor;
    result.ScaleFactor = 1;
    result.HasFastScreenBlit = _display.hasFastScreenBlit;

    SendReply(
      request.ReplyPortID,
      &result,
      sizeof(result)
    );
  }

  void DisplayController::_handleGetBackBuffer(
    const GraphicsGetBackBufferRequest& request
  ) {
    GraphicsGetBackBufferResult result;

    result.BufferID
      = _display.hasDirectFramebuffer
      ? _display.framebufferBufferID
      : _display.backBuffer.GetBufferID();
    result.Width = _display.screenWidth;
    result.Height = _display.screenHeight;
    result.ScaleFactor = 1;
    result.HardwareCursor = _display.hasHardwareCursor;
    result.HasFastScreenBlit = _display.hasFastScreenBlit;
    result.BitsPerPixel = _display.bitsPerPixel;
    result.DirectFramebuffer = _display.hasDirectFramebuffer;

    SendReply(
      request.ReplyPortID,
      &result,
      sizeof(result)
    );
  }

  void DisplayController::_handleScreenBlit(
    const GraphicsScreenBlitRequest& request
  ) {
    // skip backBuffer.ScreenBlit(): the caller (AppServer) already
    // updated the shared back buffer via direct memory writes;
    // blitting the back buffer here would copy stale strip content
    // (background) over the correctly-placed window pixels

    _display.driver->ScreenBlit(
      request.SourceX,
      request.SourceY,
      request.DestinationX,
      request.DestinationY,
      request.Width,
      request.Height
    );

    if (_display.IsInBatchMode()) {
      _display.ExpandDirtyRectangle(
        request.DestinationX,
        request.DestinationY,
        request.Width,
        request.Height
      );
    }

    GraphicsServerRequest acknowledgment;

    acknowledgment.ABIVersion = GraphicsABIVersion;
    acknowledgment.Operation = GraphicsServerOperation::ScreenBlit;

    _flushAckCache.Send(
      request.ReplyPortID,
      &acknowledgment,
      sizeof(acknowledgment)
    );
  }

  void DisplayController::_handleFlushBackBuffer(
    const GraphicsFlushBackBufferRequest& request
  ) {
    if (
      !_display.hasDirectFramebuffer &&
      _display.backBuffer.GetBuffer()
    ) {
      UInt16 flushX = request.X;
      UInt16 flushY = request.Y;
      UInt16 flushWidth = request.Width;
      UInt16 flushHeight = request.Height;

      if (
        flushX < _display.screenWidth &&
        flushY < _display.screenHeight
      ) {
        if (flushX + flushWidth > _display.screenWidth) {
          flushWidth = _display.screenWidth - flushX;
        }

        if (flushY + flushHeight > _display.screenHeight) {
          flushHeight = _display.screenHeight - flushY;
        }

        bool overlap
          = !_display.hasHardwareCursor
         && _cursor.IsVisible()
         && _cursor.Overlaps(
              flushX,
              flushY,
              flushWidth,
              flushHeight
            );

        _display.driver->WriteNativeRegion(
          flushX,
          flushY,
          flushWidth,
          flushHeight,
          _display.screenWidth,
          _display.backBuffer.GetBuffer()
        );

        if (overlap) {
          _display.driver->SetBatchMode(true);
          _cursor.Draw(_display);
          _display.driver->SetBatchMode(false);
        }

        _display.driver->FlushRegion(
          flushX,
          flushY,
          flushWidth,
          flushHeight
        );
      }
    }

    GraphicsServerRequest acknowledgment;

    acknowledgment.ABIVersion = GraphicsABIVersion;
    acknowledgment.Operation
      = GraphicsServerOperation::FlushBackBuffer;

    _flushAckCache.Send(
      request.ReplyPortID,
      &acknowledgment,
      sizeof(acknowledgment)
    );
  }

  void DisplayController::_handleBeginBatch(
    const GraphicsBeginBatchRequest& request
  ) {
    _display.BeginBatch();
  }

  void DisplayController::_handleEndBatch(
    const GraphicsEndBatchRequest& request
  ) {
    _display.EndBatch();
  }
}
