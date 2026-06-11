/**
 * @file Servers/App/Overlays/OverlayManager.cpp
 * @brief Implements @ref @QAppSrv::Overlays::OverlayManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "OverlayManager.hpp"

namespace Quantum::Servers::App::Overlays {
  Size OverlayManager::AllocateOverlay() {
    for (Size index = 0; index < MaxOverlays; ++index) {
      if (_overlays[index].ID == 0) {
        _overlays[index].ID = _nextID++;

        return index;
      }
    }

    return static_cast<Size>(-1);
  }

  void OverlayManager::FreeOverlay(Size index) {
    if (index >= MaxOverlays) return;

    _overlays[index] = Overlay{};
  }

  Overlay* OverlayManager::FindByID(UInt32 id) {
    for (Size index = 0; index < MaxOverlays; ++index) {
      if (_overlays[index].ID == id) {
        return &_overlays[index];
      }
    }

    return nullptr;
  }

  Overlay* OverlayManager::HitTest(Int16 x, Int16 y) {
    Overlay* hit = nullptr;

    for (Size index = 0; index < MaxOverlays; ++index) {
      if (_overlays[index].ID == 0) continue;

      if (_overlays[index].HitTest(x, y)) {
        hit = &_overlays[index];
      }
    }

    return hit;
  }

  void OverlayManager::DrawOverlays(
    Rectangle strip,
    const DrawContext& ctx
  ) {
    if (!ctx.BlitBuffer) return;

    for (Size index = 0; index < MaxOverlays; ++index) {
      Overlay& overlay = _overlays[index];

      if (overlay.ID == 0) continue;
      if (!overlay.ContentBuffer) continue;

      Rectangle overlayClip = overlay.Frame.Intersect(strip);

      if (overlayClip.IsEmpty()) continue;

      Int16 srcX = static_cast<Int16>(
        overlayClip.Origin.X - overlay.Frame.Origin.X
      );
      Int16 srcY = static_cast<Int16>(
        overlayClip.Origin.Y - overlay.Frame.Origin.Y
      );

      UInt32 pixelOffset
        = static_cast<UInt32>(srcY) * overlay.ContentStride + srcX;
      const void* source
        = static_cast<const UInt8*>(overlay.ContentBuffer)
        + pixelOffset * overlay.ContentBytesPerPixel;

      ctx.BlitBuffer(
        ctx.UserData,
        overlayClip.Origin.X,
        overlayClip.Origin.Y,
        source,
        overlayClip.Dimensions.Width,
        overlayClip.Dimensions.Height,
        overlay.ContentStride,
        overlay.ContentBytesPerPixel
      );
    }
  }
}
