/**
 * @file Servers/Graphics/Controllers/CursorController.cpp
 * @brief Implements @ref @QGfxSrv::CursorController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "CursorController.hpp"

namespace Quantum::Servers::Graphics {
  CursorController::CursorController(
    KernelClient& kernel,
    ServerLog& log,
    Display& display,
    Cursor& cursor
  ) :
    RequestController(
      kernel,
      log,
      Route<&CursorController::_handleSetCursorBitmap>(
        GraphicsServerOperation::SetCursorBitmap
      ),
      Route<&CursorController::_handleMoveCursor>(
        GraphicsServerOperation::MoveCursor
      ),
      Route<&CursorController::_handleShowCursor>(
        GraphicsServerOperation::ShowCursor
      )
    ),
    _display(display),
    _cursor(cursor)
  {
  }

  void CursorController::_handleSetCursorBitmap(
    const GraphicsSetCursorBitmapRequest& request,
    const IPCMessage* message
  ) {
    UInt8 width = request.Width;
    UInt8 height = request.Height;

    if (width > MaxCursorWidth) {
      width = MaxCursorWidth;
    }

    if (height > MaxCursorHeight) {
      height = MaxCursorHeight;
    }

    Size expectedSize
      = sizeof(GraphicsSetCursorBitmapRequest)
      + static_cast<Size>(width) * height * sizeof(UInt32);

    if (message->PayloadSizeInBytes >= expectedSize) {
      if (
        _cursor.IsVisible() &&
        !_display.hasHardwareCursor
      ) {
        _cursor.Undraw(_display);
      }

      _cursor.SetBitmap(
        width,
        height,
        request.TransparentColor,
        request.Pixels
      );

      if (_display.hasHardwareCursor) {
        _display.driver->SetHardwareCursorBitmap(
          width,
          height,
          request.TransparentColor,
          request.Pixels
        );
      } else if (_cursor.IsVisible()) {
        _cursor.Draw(_display);
      }
    }
  }

  void CursorController::_handleMoveCursor(
    const GraphicsMoveCursorRequest& request
  ) {
    if (
      request.X != _cursor.GetX() ||
      request.Y != _cursor.GetY()
    ) {
      if (_display.hasHardwareCursor) {
        _cursor.SetPosition(request.X, request.Y);

        _display.driver->SetHardwareCursorPosition(
          request.X,
          request.Y
        );
      } else if (request.SuppressFlush) {
        _cursor.SetPosition(request.X, request.Y);
      } else if (_cursor.IsVisible()) {
        Cursor::VisibleRect oldRect
          = _cursor.ComputeVisibleRect(
              _display.screenWidth,
              _display.screenHeight
            );

        _display.driver->SetBatchMode(true);
        _cursor.Undraw(_display);

        _cursor.SetPosition(request.X, request.Y);

        _cursor.Draw(_display);
        _display.driver->SetBatchMode(false);

        Cursor::VisibleRect newRect
          = _cursor.ComputeVisibleRect(
              _display.screenWidth,
              _display.screenHeight
            );

        bool oldHasArea
          = oldRect.VisibleWidth > 0 && oldRect.VisibleHeight > 0;
        bool newHasArea
          = newRect.VisibleWidth > 0 && newRect.VisibleHeight > 0;

        if (oldHasArea && newHasArea) {
          UInt16 unionX1 = oldRect.X < newRect.X
            ? oldRect.X
            : newRect.X;
          UInt16 unionY1 = oldRect.Y < newRect.Y
            ? oldRect.Y
            : newRect.Y;
          UInt16 oldRight
            = static_cast<UInt16>(oldRect.X + oldRect.VisibleWidth);
          UInt16 newRight
            = static_cast<UInt16>(newRect.X + newRect.VisibleWidth);
          UInt16 oldBottom
            = static_cast<UInt16>(oldRect.Y + oldRect.VisibleHeight);
          UInt16 newBottom
            = static_cast<UInt16>(newRect.Y + newRect.VisibleHeight);
          UInt16 unionX2 = oldRight > newRight ? oldRight : newRight;
          UInt16 unionY2
            = oldBottom > newBottom ? oldBottom : newBottom;

          _display.driver->FlushRegion(
            unionX1,
            unionY1,
            static_cast<UInt16>(unionX2 - unionX1),
            static_cast<UInt16>(unionY2 - unionY1)
          );
        } else if (oldHasArea) {
          _display.driver->FlushRegion(
            oldRect.X,
            oldRect.Y,
            oldRect.VisibleWidth,
            oldRect.VisibleHeight
          );
        } else if (newHasArea) {
          _display.driver->FlushRegion(
            newRect.X,
            newRect.Y,
            newRect.VisibleWidth,
            newRect.VisibleHeight
          );
        }
      } else {
        _cursor.SetPosition(request.X, request.Y);
      }
    }
  }

  void CursorController::_handleShowCursor(
    const GraphicsShowCursorRequest& request
  ) {
    if (
      request.Visible &&
      !_cursor.IsVisible()
    ) {
      _display.EndBatch();

      _cursor.SetVisible(true);

      if (_display.hasHardwareCursor) {
        _display.driver->SetHardwareCursorVisible(true);
      } else {
        _cursor.Draw(_display);
      }
    } else if (
      !request.Visible &&
      _cursor.IsVisible()
    ) {
      if (_display.hasHardwareCursor) {
        _display.driver->SetHardwareCursorVisible(false);
      } else {
        _cursor.Undraw(_display);
      }

      _cursor.SetVisible(false);

      _display.BeginBatch();
    }
  }
}
