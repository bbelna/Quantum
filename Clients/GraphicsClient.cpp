/**
 * @file Clients/GraphicsClient.cpp
 * @brief Implements @ref @QClients::GraphicsClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "GraphicsClient.hpp"

namespace Quantum::Clients {
  GraphicsClient::GraphicsClient(
    IPCPortResourceID gfxHandle,
    IPCPortResourceID flushReplyHandle,
    IPCPortID flushReplyPortID,
    UInt8 scaleFactor
  ) :
    _gfxHandle(gfxHandle),
    _gfxFlushReplyHandle(flushReplyHandle),
    _gfxFlushReplyPortID(flushReplyPortID),
    _scaleFactor(scaleFactor)
  {
  }

  void GraphicsClient::SetSharedState(const GraphicsClientShared& shared) {
    _shared = shared;
  }

  void GraphicsClient::FillRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    GraphicsFillRectangleRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::FillRectangle;
    request.X = static_cast<UInt16>(x * _scaleFactor);
    request.Y = static_cast<UInt16>(y * _scaleFactor);
    request.Width = static_cast<UInt16>(w * _scaleFactor);
    request.Height = static_cast<UInt16>(h * _scaleFactor);
    request.Color = color;

    SendOS(GraphicsPortID, request);
  }

  void GraphicsClient::XORRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    UInt32 color
  ) {
    GraphicsXORRectangleRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::XORRectangle;
    request.X = static_cast<UInt16>(x * _scaleFactor);
    request.Y = static_cast<UInt16>(y * _scaleFactor);
    request.Width = static_cast<UInt16>(w * _scaleFactor);
    request.Height = static_cast<UInt16>(h * _scaleFactor);
    request.Color = color;

    SendOS(GraphicsPortID, request);
  }

  void GraphicsClient::ScreenBlit(
    UInt16 srcX,
    UInt16 srcY,
    UInt16 dstX,
    UInt16 dstY,
    UInt16 w,
    UInt16 h
  ) {
    GraphicsScreenBlitRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::ScreenBlit;
    request.ReplyPortID = 0;
    request.SourceX = static_cast<UInt16>(srcX * _scaleFactor);
    request.SourceY = static_cast<UInt16>(srcY * _scaleFactor);
    request.DestinationX = static_cast<UInt16>(dstX * _scaleFactor);
    request.DestinationY = static_cast<UInt16>(dstY * _scaleFactor);
    request.Width = static_cast<UInt16>(w * _scaleFactor);
    request.Height = static_cast<UInt16>(h * _scaleFactor);

    _kernel.SendIPCMessage(_gfxHandle, &request, sizeof(request));
  }

  void GraphicsClient::ScreenBlitSync(
    UInt16 srcX,
    UInt16 srcY,
    UInt16 dstX,
    UInt16 dstY,
    UInt16 w,
    UInt16 h
  ) {
    GraphicsScreenBlitRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::ScreenBlit;
    request.ReplyPortID = _gfxFlushReplyPortID;
    request.SourceX = static_cast<UInt16>(srcX * _scaleFactor);
    request.SourceY = static_cast<UInt16>(srcY * _scaleFactor);
    request.DestinationX = static_cast<UInt16>(dstX * _scaleFactor);
    request.DestinationY = static_cast<UInt16>(dstY * _scaleFactor);
    request.Width = static_cast<UInt16>(w * _scaleFactor);
    request.Height = static_cast<UInt16>(h * _scaleFactor);

    _kernel.SendIPCMessage(_gfxHandle, &request, sizeof(request));

    IPCMessage* ack = _kernel.ReceiveIPCMessage(_gfxFlushReplyHandle);

    if (ack) free(ack);
  }

  void GraphicsClient::MoveCursor(Point p, bool suppressFlush) {
    GraphicsMoveCursorRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::MoveCursor;
    request.X = static_cast<Int16>(p.X * _scaleFactor);
    request.Y = static_cast<Int16>(p.Y * _scaleFactor);
    request.SuppressFlush = suppressFlush;

    SendOS(GraphicsPortID, request);
  }

  void GraphicsClient::ShowCursor(bool visible) {
    GraphicsShowCursorRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::ShowCursor;
    request.Visible = visible;

    SendOS(GraphicsPortID, request);
  }

  void GraphicsClient::BeginBatch() {
    GraphicsRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::BeginBatch;

    SendOS(GraphicsPortID, request);
  }

  void GraphicsClient::EndBatch() {
    GraphicsRequest request = {};

    request.ABIVersion = GraphicsABIVersion;
    request.Operation = GraphicsOperation::EndBatch;

    SendOS(GraphicsPortID, request);
  }

  void GraphicsClient::FlushBackBuffer(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h,
    bool sync
  ) {
    if (*_shared.DirectFramebuffer) {
      if (
        *_shared.CompositeBuffer &&
        *_shared.CompositeBuffer != *_shared.SharedBuffer
      ) CopyToShared(x, y, w, h);

      return;
    }

    WaitForPendingFlush();

    if (
      *_shared.SharedBuffer &&
      *_shared.CompositeBuffer != *_shared.SharedBuffer
    ) CopyToShared(x, y, w, h);

    UInt16 sx = static_cast<UInt16>(x * _scaleFactor);
    UInt16 sy = static_cast<UInt16>(y * _scaleFactor);
    UInt16 sw = static_cast<UInt16>(w * _scaleFactor);
    UInt16 sh = static_cast<UInt16>(h * _scaleFactor);

    if (
      sync &&
      _gfxFlushReplyHandle != static_cast<IPCPortResourceID>(-1)
    ) {
      GraphicsFlushBackBufferRequest request = {};

      request.ABIVersion = GraphicsABIVersion;
      request.Operation = GraphicsOperation::FlushBackBuffer;
      request.ReplyPortID = _gfxFlushReplyPortID;
      request.X = sx;
      request.Y = sy;
      request.Width = sw;
      request.Height = sh;

      _kernel.SendIPCMessage(_gfxHandle, &request, sizeof(request));

      _pendingFlushCount++;
      _flushPending = true;
    } else {
      GraphicsFlushBackBufferRequest request = {};

      request.ABIVersion = GraphicsABIVersion;
      request.Operation = GraphicsOperation::FlushBackBuffer;
      request.ReplyPortID = 0;
      request.X = sx;
      request.Y = sy;
      request.Width = sw;
      request.Height = sh;

      _kernel.SendIPCMessage(_gfxHandle, &request, sizeof(request));
    }

    if (*_shared.DebugFlushRegion && sw > 2 && sh > 2) {
      XORRectangle(sx, sy, sw, 1, 0xFFFF00FF);
      XORRectangle(
        sx,
        static_cast<UInt16>(sy + sh - 1),
        sw,
        1,
        0xFFFF00FF
      );
      XORRectangle(
        sx,
        static_cast<UInt16>(sy + 1),
        1,
        static_cast<UInt16>(sh - 2),
        0xFFFF00FF
      );
      XORRectangle(
        static_cast<UInt16>(sx + sw - 1),
        static_cast<UInt16>(sy + 1),
        1,
        static_cast<UInt16>(sh - 2),
        0xFFFF00FF
      );
    }
  }

  void GraphicsClient::WaitForPendingFlush() {
    if (!_flushPending) return;

    while (_pendingFlushCount > 0) {
      IPCMessage* ack = _kernel.TryReceiveIPCMessage(
        _gfxFlushReplyHandle
      );

      if (!ack) ack = _kernel.ReceiveIPCMessage(_gfxFlushReplyHandle);
      if (ack) delete ack;

      _pendingFlushCount--;
    }

    _flushPending = false;
  }

  bool GraphicsClient::CopyToShared(
    UInt16 x,
    UInt16 y,
    UInt16 w,
    UInt16 h
  ) {
    if (
      !*_shared.SharedBuffer ||
      !*_shared.CompositeBuffer ||
      *_shared.CompositeBuffer == *_shared.SharedBuffer
    ) return true;

    UInt16 screenWidth = *_shared.ScreenWidth;
    UInt16 screenHeight = *_shared.ScreenHeight;
    UInt16 bufferWidth = *_shared.BufferWidth;
    UInt16 x2 = static_cast<UInt16>(x + w);
    UInt16 y2 = static_cast<UInt16>(y + h);

    if (x2 > screenWidth) x2 = screenWidth;
    if (y2 > screenHeight) y2 = screenHeight;
    if (x >= x2 || y >= y2) return true;

    UInt16 cw = static_cast<UInt16>(x2 - x);

    UInt32* src = static_cast<UInt32*>(*_shared.CompositeBuffer);
    UInt32* dst = static_cast<UInt32*>(*_shared.SharedBuffer);

    for (UInt16 row = y; row < y2; ++row) {
      UInt32 off = static_cast<UInt32>(row) * bufferWidth + x;
      void* d = &dst[off];
      const void* s = &src[off];
      UInt32 cnt = cw;

      asm volatile(
        "cld\n"
        "rep movsl"
        : "+D"(d), "+S"(s), "+c"(cnt)
        :
        : "memory"
      );

      if (*_shared.RenderSignal) return false;
    }

    return true;
  }

  void GraphicsClient::DrawText(
    Int16 x,
    Int16 y,
    const char* title,
    Size maxLen,
    UInt32 color,
    UInt16 fontWeight,
    UInt8 glyphWidth,
    UInt8 glyphHeight
  ) {
    constexpr Size MaxTitle = 64;

    if (!title || !title[0] || maxLen == 0) return;

    // compute effective length, capped by maxLen and null terminator
    Size length = 0;

    while (title[length] && length < maxLen) length++;

    if (length == 0) return;

    // skip glyphs that start completely off-screen to the left
    if (x < 0) {
      Int16 skipChars = static_cast<Int16>((-x + glyphWidth - 1) / glyphWidth);

      if (skipChars >= static_cast<Int16>(length)) return;

      title += skipChars;
      length -= static_cast<Size>(skipChars);
      x = static_cast<Int16>(x + skipChars * glyphWidth);
    }

    UInt16 screenHeight = *_shared.ScreenHeight;
    UInt16 screenWidth = *_shared.ScreenWidth;

    if (
      y < -(glyphHeight - 1) ||
      y >= static_cast<Int16>(screenHeight) ||
      x >= static_cast<Int16>(screenWidth)
    ) return;

    if (length > MaxTitle) length = MaxTitle;

    Size requestSize = sizeof(GraphicsDrawTextRequest) + length + 1;
    UIntPtr allocation = _kernel.AllocateBlock(requestSize);

    if (allocation == 0) return;

    GraphicsDrawTextRequest* request
      = reinterpret_cast<GraphicsDrawTextRequest*>(
        allocation
      );

    request->ABIVersion = GraphicsABIVersion;
    request->Operation = GraphicsOperation::DrawText;
    request->X = static_cast<UInt16>(x * _scaleFactor);
    request->Y = static_cast<UInt16>(y * _scaleFactor);
    request->ForegroundColor = color;
    request->BackgroundColor = 0x00000000;
    request->FontSize = 0;
    request->FontWeight = fontWeight;
    request->TextLength = length;

    Byte::Copy(request->Text, title, length + 1);

    _kernel.SendIPCMessage(_gfxHandle, request, requestSize);
    _kernel.FreeBlock(allocation);
  }

  void GraphicsClient::SetCursorBitmap(const UInt32* pixels) {
    Size pixelCount = static_cast<Size>(CursorWidth) * CursorHeight;
    Size pixelBytes = pixelCount * sizeof(UInt32);
    Size requestSize = sizeof(GraphicsSetCursorBitmapRequest) + pixelBytes;

    UIntPtr allocation = _kernel.AllocateBlock(requestSize);

    if (allocation == 0) return;

    GraphicsSetCursorBitmapRequest* request
      = reinterpret_cast<GraphicsSetCursorBitmapRequest*>(
        allocation
      );

    request->ABIVersion = GraphicsABIVersion;
    request->Operation = GraphicsOperation::SetCursorBitmap;
    request->Width = CursorWidth;
    request->Height = CursorHeight;
    request->TransparentColor = CursorTransparent;

    Byte::Copy(request->Pixels, pixels, pixelBytes);

    _kernel.SendIPCMessage(_gfxHandle, request, requestSize);
    _kernel.FreeBlock(allocation);
  }

  void GraphicsClient::ClippedFillRectangle(
    Rectangle rectangle,
    UInt32 color
  ) {
    UInt16 screenWidth = *_shared.ScreenWidth;
    UInt16 screenHeight = *_shared.ScreenHeight;
    Rectangle screen(0, 0, screenWidth, screenHeight);
    Rectangle clamped = rectangle.Intersect(screen);

    if (clamped.IsEmpty()) return;

    FillRectangle(
      static_cast<UInt16>(clamped.Origin.X),
      static_cast<UInt16>(clamped.Origin.Y),
      clamped.Dimensions.Width,
      clamped.Dimensions.Height,
      color
    );
  }

  void GraphicsClient::DrawOutline(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height
  ) {
    // compute the rectangle edges in signed coordinates
    Int16 x2 = static_cast<Int16>(x + width);
    Int16 y2 = static_cast<Int16>(y + height);

    // clamp the left/top edges to the screen origin
    Int16 cx = static_cast<Int16>((x < 0) ? 0 : x);
    Int16 cy = static_cast<Int16>((y < 0) ? 0 : y);

    // skip if fully off-screen
    if (x2 <= 0 || y2 <= 0) return;
    if (cx >= x2 || cy >= y2) return;

    UInt16 ux = static_cast<UInt16>(cx);
    UInt16 uy = static_cast<UInt16>(cy);
    UInt16 cw = static_cast<UInt16>(x2 - cx);
    UInt16 ch = static_cast<UInt16>(y2 - cy);

    // top edge (only if the original top is visible)
    if (y >= 0) XORRectangle(ux, uy, cw, 1, OutlineColor);

    // bottom edge
    XORRectangle(
      ux,
      static_cast<UInt16>(y2 - 1),
      cw,
      1,
      OutlineColor
    );

    // left edge (only if the original left is visible)
    if (x >= 0) {
      XORRectangle(
        ux,
        static_cast<UInt16>(uy + (y >= 0 ? 1 : 0)),
        1,
        static_cast<UInt16>(ch - (y >= 0 ? 2 : 1)),
        OutlineColor
      );
    }

    // right edge
    XORRectangle(
      static_cast<UInt16>(x2 - 1),
      static_cast<UInt16>(uy + (y >= 0 ? 1 : 0)),
      1,
      static_cast<UInt16>(ch - (y >= 0 ? 2 : 1)),
      OutlineColor
    );
  }
}
