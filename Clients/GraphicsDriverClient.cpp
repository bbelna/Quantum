/**
 * @file Clients/GraphicsDriverClient.cpp
 * @brief Implements @ref @QClients::GraphicsDriverClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "GraphicsDriverClient.hpp"

namespace Quantum::Clients {
  ModeInfoPayload GraphicsDriverClient::QueryModeInfo() {
    ModeInfoPayload payload = {};

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::GetModeInfo),
      &payload
    );

    return payload;
  }

  bool GraphicsDriverClient::HasHardwareCursor() {
    HardwareCursorSupportPayload payload = {};

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::GetHardwareCursorSupport),
      &payload
    );

    return payload.Supported;
  }

  bool GraphicsDriverClient::HasFastScreenBlit() {
    FastScreenBlitSupportPayload payload = {};

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::GetFastScreenBlitSupport),
      &payload
    );

    return payload.Supported;
  }

  UInt32 GraphicsDriverClient::GetFramebufferBufferID() {
    FramebufferBufferIDPayload payload = {};

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::GetFramebufferBufferID),
      &payload
    );

    return payload.BufferID;
  }

  void GraphicsDriverClient::WriteText(char* text) {
    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::WriteText),
      static_cast<void*>(text)
    );
  }

  void GraphicsDriverClient::SetMode(UInt16 mode) {
    SetModePayload payload { mode };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::SetMode),
      &payload
    );
  }

  void GraphicsDriverClient::FillRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 color
  ) {
    FillRectanglePayload payload {
      x,
      y,
      width,
      height,
      color
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::FillRectangle),
      &payload
    );
  }

  void GraphicsDriverClient::BlitBuffer(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 transparent,
    const UInt32* pixels
  ) {
    BlitBufferPayload payload {
      x,
      y,
      width,
      height,
      transparent,
      pixels
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::BlitBuffer),
      &payload
    );
  }

  void GraphicsDriverClient::XORRectangle(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 color
  ) {
    XORRectanglePayload payload {
      x,
      y,
      width,
      height,
      color
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::XORRectangle),
      &payload
    );
  }

  void GraphicsDriverClient::ScreenBlit(
    UInt16 sourceX,
    UInt16 sourceY,
    UInt16 destinationX,
    UInt16 destinationY,
    UInt16 width,
    UInt16 height
  ) {
    ScreenBlitPayload payload {
      sourceX,
      sourceY,
      destinationX,
      destinationY,
      width,
      height
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::ScreenBlit),
      &payload
    );
  }

  void GraphicsDriverClient::WritePixelRegion(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 sourcePitch,
    const UInt32* pixels
  ) {
    WritePixelRegionPayload payload {
      x,
      y,
      width,
      height,
      sourcePitch,
      pixels
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::WritePixelRegion),
      &payload
    );
  }

  void GraphicsDriverClient::WriteNativeRegion(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height,
    UInt32 sourcePitch,
    const void* pixels
  ) {
    WriteNativeRegionPayload payload {
      x,
      y,
      width,
      height,
      sourcePitch,
      pixels
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::WriteNativeRegion),
      &payload
    );
  }

  void GraphicsDriverClient::SetBatchMode(bool enabled) {
    SetBatchModePayload payload { enabled };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::SetBatchMode),
      &payload
    );
  }

  void GraphicsDriverClient::FlushRegion(
    UInt16 x,
    UInt16 y,
    UInt16 width,
    UInt16 height
  ) {
    FlushRegionPayload payload {
      x,
      y,
      width,
      height
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::FlushRegion),
      &payload
    );
  }

  void GraphicsDriverClient::SetHardwareCursorBitmap(
    UInt8 width,
    UInt8 height,
    UInt32 transparent,
    const UInt32* pixels
  ) {
    SetHardwareCursorBitmapPayload payload {
      width,
      height,
      transparent,
      pixels
    };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::SetHardwareCursorBitmap),
      &payload
    );
  }

  void GraphicsDriverClient::SetHardwareCursorPosition(Int16 x, Int16 y) {
    SetHardwareCursorPositionPayload payload { x, y };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::SetHardwareCursorPosition),
      &payload
    );
  }

  void GraphicsDriverClient::SetHardwareCursorVisible(bool visible) {
    SetHardwareCursorVisiblePayload payload { visible };

    InvokeDriver(
      Enum::ToBase(GraphicsDriverOperation::SetHardwareCursorVisible),
      &payload
    );
  }
}
