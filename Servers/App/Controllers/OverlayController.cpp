/**
 * @file Servers/App/Controllers/OverlayController.cpp
 * @brief Implements @ref @QAppSrv::Controllers::OverlayController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <AppServerTypes.hpp>

#include "OverlayController.hpp"
#include "Compositor.hpp"

namespace Quantum::Servers::App::Controllers {
  OverlayController::OverlayController(
    KernelClient& kernel,
    ServerLog& log,
    Compositor& compositor
  ) :
    RequestController(kernel, log),
    _compositor(compositor)
  {
  }

  void OverlayController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (static_cast<ABI::Operation>(operation)) {
      case ABI::Operation::CreateOverlay:
        _handleCreateOverlay(message);
        break;

      case ABI::Operation::CloseOverlay:
        _handleCloseOverlay(message);
        break;

      case ABI::Operation::InvalidateOverlay:
        _handleInvalidateOverlay(message);
        break;

      case ABI::Operation::GetOverlayEvent:
        _handleGetOverlayEvent(message);
        break;

      case ABI::Operation::SetOverlayPosition:
        _handleSetOverlayPosition(message);
        break;

      default:
        _log.Warning("Unknown overlay operation %u", operation);
        break;
    }
  }

  void OverlayController::_handleCreateOverlay(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::CreateOverlayRequest)
    ) {
      return;
    }

    auto* req = static_cast<const ABI::CreateOverlayRequest*>(
      message->Payload
    );

    Size index = _overlayManager.AllocateOverlay();

    if (index == static_cast<Size>(-1)) {
      ABI::CreateOverlayResult failResult = {};

      failResult.Success = false;

      SendReply(req->ReplyPortID, &failResult, sizeof(failResult));

      return;
    }

    Overlay& overlay = _overlayManager.GetOverlay(index);

    overlay.Frame = Rectangle(req->X, req->Y, req->Width, req->Height);
    overlay.ContentWidth = req->Width;
    overlay.ContentHeight = req->Height;
    overlay.ContentStride = req->Width;
    overlay.ContentBytesPerPixel = 4;

    UInt32 bufferSize = static_cast<UInt32>(req->Width)
      * req->Height * 4;

    overlay.ContentBufferID = _kernel.CreateSharedBuffer(bufferSize);

    if (overlay.ContentBufferID != 0) {
      UIntPtr bufferAddress
        = _kernel.AttachSharedBuffer(overlay.ContentBufferID);

      if (bufferAddress != 0) {
        overlay.ContentBuffer = reinterpret_cast<void*>(bufferAddress);

        // overlays are always ARGB32; init directly
        UInt32* overlayPixels
          = static_cast<UInt32*>(overlay.ContentBuffer);
        UInt32 overlayPixelCount
          = static_cast<UInt32>(req->Width) * req->Height;

        for (UInt32 p = 0; p < overlayPixelCount; ++p) {
          overlayPixels[p] = 0xFFEEEEEE;
        }
      }
    }

    _compositor.Damage(overlay.Frame);

    // if the overlay is at the top of the screen (menu bar), also
    // damage the shadow area below it so the compositor draws the
    // menu bar shadow on the initial composite
    if (overlay.Frame.Origin.Y == 0) {
      UInt16 shadowBottom = static_cast<UInt16>(
        overlay.Frame.Dimensions.Height + Theme::ShadowSize
      );

      _compositor.Damage(Rectangle(
        0,
        static_cast<Int16>(overlay.Frame.Dimensions.Height),
        overlay.Frame.Dimensions.Width,
        static_cast<UInt16>(shadowBottom - overlay.Frame.Dimensions.Height)
      ));
    }

    ABI::CreateOverlayResult result = {};

    result.Success = true;
    result.ID = overlay.ID;
    result.ContentBufferID = overlay.ContentBufferID;
    result.ContentWidth = overlay.ContentWidth;
    result.ContentHeight = overlay.ContentHeight;
    result.ContentStride = overlay.ContentStride;
    result.ContentBytesPerPixel = overlay.ContentBytesPerPixel;

    SendReply(req->ReplyPortID, &result, sizeof(result));
  }

  void OverlayController::_handleCloseOverlay(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::CloseOverlayRequest)
    ) {
      return;
    }

    auto* req = static_cast<const ABI::CloseOverlayRequest*>(
      message->Payload
    );

    Overlay* overlay = _overlayManager.FindByID(req->ID);

    if (!overlay) return;

    _compositor.Damage(overlay->Frame);

    if (overlay->ContentBuffer) {
      _kernel.DetachSharedBuffer(
        reinterpret_cast<UIntPtr>(overlay->ContentBuffer)
      );
    }

    for (Size index = 0; index < _overlayManager.GetCapacity(); ++index) {
      if (_overlayManager.GetOverlay(index).ID == req->ID) {
        _overlayManager.FreeOverlay(index);

        break;
      }
    }
  }

  void OverlayController::_handleInvalidateOverlay(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::InvalidateOverlayRequest)
    ) {
      return;
    }

    auto* req = static_cast<const ABI::InvalidateOverlayRequest*>(
      message->Payload
    );

    Overlay* overlay = _overlayManager.FindByID(req->ID);

    if (!overlay) return;

    _compositor.Damage(overlay->Frame);
  }

  void OverlayController::_handleGetOverlayEvent(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::GetOverlayEventRequest)
    ) {
      return;
    }

    auto* req = static_cast<const ABI::GetOverlayEventRequest*>(
      message->Payload
    );

    Overlay* overlay = _overlayManager.FindByID(req->ID);

    if (!overlay) return;

    // if there's a queued event, deliver it immediately
    if (overlay->EventQueueCount > 0) {
      ABI::WindowEventResult& queued
        = overlay->EventQueue[overlay->EventQueueHead];

      SendReply(req->ReplyPortID, &queued, sizeof(queued));

      overlay->EventQueueHead
        = (overlay->EventQueueHead + 1) % Overlay::EventQueueCapacity;
      overlay->EventQueueCount--;
    } else {
      // no queued events, store the reply port for later delivery
      overlay->EventReplyPortID = req->ReplyPortID;
    }
  }

  void OverlayController::_handleSetOverlayPosition(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes < sizeof(ABI::SetOverlayPositionRequest)
    ) {
      return;
    }

    auto* req = static_cast<const ABI::SetOverlayPositionRequest*>(
      message->Payload
    );

    Overlay* overlay = _overlayManager.FindByID(req->ID);

    if (!overlay) return;

    _compositor.Damage(overlay->Frame);

    overlay->Frame.Origin.X = req->X;
    overlay->Frame.Origin.Y = req->Y;

    _compositor.Damage(overlay->Frame);
  }
}
