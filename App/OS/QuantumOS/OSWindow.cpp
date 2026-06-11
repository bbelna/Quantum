/**
 * @file App/QuantumOSWindow.cpp
 * @brief Implements @ref @QApp::OS::QuantumOS::OSWindow.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "OSWindow.hpp"

namespace Quantum::App::OS::QuantumOS {
  bool OSWindow::Create(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height,
    const char* title,
    Canvas* surface,
    UInt32 contentColor,
    bool maximized,
    bool allowClose,
    bool allowMaximize,
    bool allowResize,
    UInt32 innerContentColor
  ) {
    AppServer::CreateWindowResult result = {};

    bool valid = AppServer::CreateWindow(
      x,
      y,
      width,
      height,
      title,
      &_id,
      &result,
      contentColor,
      maximized,
      allowClose,
      allowMaximize,
      allowResize,
      innerContentColor
    );

    if (valid && result.ContentBufferID != 0) {
      UIntPtr bufferAddress = AttachShared(result.ContentBufferID);

      if (bufferAddress != 0) {
        _contentBufferAddress = bufferAddress;
        _contentBufferID = result.ContentBufferID;

        UInt8 bytesPerPixel = result.ContentBytesPerPixel;

        if (bytesPerPixel != 2 && bytesPerPixel != 4) {
          bytesPerPixel = 4;
        }

        _contentBufferCapacity
          = static_cast<UInt32>(result.ContentStride)
          * result.ContentHeight
          * bytesPerPixel;

        *surface = Canvas(
          reinterpret_cast<void*>(_contentBufferAddress),
          result.ContentWidth,
          result.ContentHeight,
          result.ContentStride,
          bytesPerPixel
        );
      }
    }

    return valid;
  }

  void OSWindow::Destroy() {
    if (_contentBufferAddress != 0) {
      DetachShared(_contentBufferAddress);

      _contentBufferAddress = 0;
    }

    AppServer::CloseWindow(_id);
  }

  void OSWindow::SetPosition(Int16 x, Int16 y) {
    AppServer::SetPosition(_id, x, y);
  }

  void OSWindow::SetSize(UInt16 width, UInt16 height) {
    AppServer::SetSize(_id, width, height);
  }

  void OSWindow::Invalidate() {
    AppServer::InvalidateContent(
      _id,
      0,
      0,
      0,
      0,
      _pendingContentBufferID,
      _pendingContentWidth,
      _pendingContentHeight,
      _pendingContentStride,
      _pendingContentBPP
    );

    _pendingContentBufferID = 0;
  }

  void OSWindow::Invalidate(Rectangle dirtyRect) {
    AppServer::InvalidateContent(
      _id,
      static_cast<UInt16>(dirtyRect.Origin.X < 0 ? 0 : dirtyRect.Origin.X),
      static_cast<UInt16>(dirtyRect.Origin.Y < 0 ? 0 : dirtyRect.Origin.Y),
      dirtyRect.Dimensions.Width,
      dirtyRect.Dimensions.Height,
      _pendingContentBufferID,
      _pendingContentWidth,
      _pendingContentHeight,
      _pendingContentStride,
      _pendingContentBPP
    );

    _pendingContentBufferID = 0;
  }

  void OSWindow::InvalidateWithShift(
    Int16 shiftDeltaY,
    Rectangle dirtyRectangle
  ) {
    AppServer::InvalidateContentShift(
      _id,
      shiftDeltaY,
      static_cast<UInt16>(
        dirtyRectangle.Origin.X < 0
          ? 0
          : dirtyRectangle.Origin.X
      ),
      static_cast<UInt16>(
        dirtyRectangle.Origin.Y < 0
          ? 0
          : dirtyRectangle.Origin.Y
      ),
      dirtyRectangle.Dimensions.Width,
      dirtyRectangle.Dimensions.Height
    );
  }

  bool OSWindow::GetWindowEvent(WindowEvent* outEvent, Canvas* surface) {
    AppServer::WindowEventResult abiResult = {};

    if (!AppServer::GetWindowEvent(_id, &abiResult)) {
      return false;
    }

    outEvent->Type = static_cast<WindowEventType>(abiResult.Type);
    outEvent->KeyboardEvent = abiResult.KeyboardEvent;
    outEvent->ContentWidth = abiResult.ContentWidth;
    outEvent->ContentHeight = abiResult.ContentHeight;
    outEvent->ScrollDelta = abiResult.ScrollDelta;
    outEvent->MouseX = abiResult.MouseX;
    outEvent->MouseY = abiResult.MouseY;
    outEvent->MouseButtons = abiResult.MouseButtons;
    outEvent->MenuActionID = abiResult.MenuActionID;

    if (
      abiResult.Type == AppServer::WindowEventType::Resize
    ) {
      UInt16 contentWidth = abiResult.ContentWidth;
      UInt16 contentHeight = abiResult.ContentHeight;
      UInt8 bytesPerPixel = abiResult.ContentBytesPerPixel;

      if (bytesPerPixel != 2 && bytesPerPixel != 4) {
        bytesPerPixel = 4;
      }

      if (contentWidth > 0 && contentHeight > 0) {
        UInt32 bufferSize
          = static_cast<UInt32>(contentWidth)
          * contentHeight
          * bytesPerPixel;

        // reuse existing buffer if large enough (no alloc, no detach,
        // no new buffer ID sent to server - just update dimensions)
        if (
          _contentBufferCapacity >= bufferSize &&
          _contentBufferAddress != 0
        ) {
          surface->Reset(
            reinterpret_cast<void*>(_contentBufferAddress),
            contentWidth, contentHeight, contentWidth, bytesPerPixel
          );

          // re-send the same buffer ID so the server updates its
          // content dimensions for this window
          _pendingContentBufferID = _contentBufferID;
          _pendingContentWidth = contentWidth;
          _pendingContentHeight = contentHeight;
          _pendingContentStride = contentWidth;
          _pendingContentBPP = bytesPerPixel;
        } else {
          SharedBufferID newBufferID = CreateShared(bufferSize);

          if (newBufferID != 0) {
            UIntPtr newAddress = AttachShared(newBufferID);

            if (newAddress != 0) {
              if (_contentBufferAddress != 0) {
                DetachShared(_contentBufferAddress);
              }

              _contentBufferAddress = newAddress;
              _contentBufferID = newBufferID;
              _contentBufferCapacity = bufferSize;

              surface->Reset(
                reinterpret_cast<void*>(_contentBufferAddress),
                contentWidth, contentHeight, contentWidth, bytesPerPixel
              );

              _pendingContentBufferID = newBufferID;
              _pendingContentWidth = contentWidth;
              _pendingContentHeight = contentHeight;
              _pendingContentStride = contentWidth;
              _pendingContentBPP = bytesPerPixel;
            }
          } else {
            if (surface->IsValid()) {
              outEvent->ContentWidth = surface->GetWidth();
              outEvent->ContentHeight = surface->GetHeight();
            }
          }
        }
      }
    }

    return true;
  }

  bool OSWindow::TryGetWindowEvent(
    WindowEvent* outEvent, Canvas* surface
  ) {
    AppServer::WindowEventResult abiResult = {};

    if (!AppServer::TryGetWindowEvent(_id, &abiResult)) {
      return false;
    }

    outEvent->Type = static_cast<WindowEventType>(abiResult.Type);
    outEvent->KeyboardEvent = abiResult.KeyboardEvent;
    outEvent->ContentWidth = abiResult.ContentWidth;
    outEvent->ContentHeight = abiResult.ContentHeight;
    outEvent->ScrollDelta = abiResult.ScrollDelta;
    outEvent->MouseX = abiResult.MouseX;
    outEvent->MouseY = abiResult.MouseY;
    outEvent->MouseButtons = abiResult.MouseButtons;
    outEvent->MenuActionID = abiResult.MenuActionID;

    if (
      abiResult.Type == AppServer::WindowEventType::Resize
    ) {
      UInt16 contentWidth = abiResult.ContentWidth;
      UInt16 contentHeight = abiResult.ContentHeight;
      UInt8 bytesPerPixel = abiResult.ContentBytesPerPixel;

      if (bytesPerPixel != 2 && bytesPerPixel != 4) {
        bytesPerPixel = 4;
      }

      if (contentWidth > 0 && contentHeight > 0) {
        UInt32 bufferSize
          = static_cast<UInt32>(contentWidth)
          * contentHeight
          * bytesPerPixel;

        // reuse existing buffer if large enough (no alloc, no detach,
        // no new buffer ID sent to server - just update dimensions)
        if (
          _contentBufferCapacity >= bufferSize &&
          _contentBufferAddress != 0
        ) {
          surface->Reset(
            reinterpret_cast<void*>(_contentBufferAddress),
            contentWidth, contentHeight, contentWidth, bytesPerPixel
          );

          // re-send the same buffer ID so the server updates its
          // content dimensions for this window
          _pendingContentBufferID = _contentBufferID;
          _pendingContentWidth = contentWidth;
          _pendingContentHeight = contentHeight;
          _pendingContentStride = contentWidth;
          _pendingContentBPP = bytesPerPixel;
        } else {
          SharedBufferID newBufferID = CreateShared(bufferSize);

          if (newBufferID != 0) {
            UIntPtr newAddress = AttachShared(newBufferID);

            if (newAddress != 0) {
              if (_contentBufferAddress != 0) {
                DetachShared(_contentBufferAddress);
              }

              _contentBufferAddress = newAddress;
              _contentBufferID = newBufferID;
              _contentBufferCapacity = bufferSize;

              surface->Reset(
                reinterpret_cast<void*>(_contentBufferAddress),
                contentWidth, contentHeight, contentWidth, bytesPerPixel
              );

              _pendingContentBufferID = newBufferID;
              _pendingContentWidth = contentWidth;
              _pendingContentHeight = contentHeight;
              _pendingContentStride = contentWidth;
              _pendingContentBPP = bytesPerPixel;
            }
          } else {
            if (surface->IsValid()) {
              outEvent->ContentWidth = surface->GetWidth();
              outEvent->ContentHeight = surface->GetHeight();
            }
          }
        }
      }
    }

    return true;
  }

  void OSWindow::SetMinimumSize(UInt16 minWidth, UInt16 minHeight) {
    AppServer::SetMinimumSize(_id, minWidth, minHeight);
  }

  void OSWindow::WaitForClose() {
    AppServer::WaitForClose(_id);
  }
}
