/**
 * @file Servers/App/Overlays/OverlayManager.hpp
 * @brief Declares @ref @QAppSrv::Overlays::OverlayManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Overlay.hpp"

namespace Quantum::Servers::App::Overlays {
  /**
   * @brief Manages the overlay table and provides lookup/iteration.
   */
  class OverlayManager {
    public:
      /**
       * @brief Creates a new overlay and returns its index.
       * @return The index into the overlay table, or
       *         `static_cast<Size>(-1)` on failure.
       */
      Size AllocateOverlay();

      /**
       * @brief Returns the overlay at the given index.
       * @param index The overlay table index.
       */
      Overlay& GetOverlay(Size index) { return _overlays[index]; }

      /**
       * @brief Returns the overlay at the given index (const).
       * @param index The overlay table index.
       */
      const Overlay& GetOverlay(Size index) const {
        return _overlays[index];
      }

      /**
       * @brief Returns the number of overlay slots (including unused).
       */
      Size GetCapacity() const { return MaxOverlays; }

      /**
       * @brief Returns the bottom edge of the topmost overlay that starts
       *        at Y=0 (i.e. the menu bar height). Returns 0 if no such
       *        overlay exists.
       */
      UInt16 GetTopOverlayBottom() const {
        for (Size index = 0; index < MaxOverlays; ++index) {
          if (_overlays[index].ID == 0) {
            continue;
          }

          if (_overlays[index].Frame.Origin.Y == 0) {
            return static_cast<UInt16>(
              _overlays[index].Frame.Dimensions.Height
            );
          }
        }

        return 0;
      }

      /**
       * @brief Frees an overlay slot.
       * @param index The overlay table index.
       */
      void FreeOverlay(Size index);

      /**
       * @brief Finds an overlay by ID.
       * @param id The overlay resource ID.
       * @return Pointer to the overlay, or `nullptr` if not found.
       */
      Overlay* FindByID(UInt32 id);

      /**
       * @brief Returns the topmost overlay at the given screen point,
       *        testing from back to front (last allocated = frontmost).
       * @param x Screen-space X coordinate.
       * @param y Screen-space Y coordinate.
       * @return Pointer to the overlay, or `nullptr` if none hit.
       */
      Overlay* HitTest(Int16 x, Int16 y);

      /**
       * @brief Draws all active overlays clipped to the given strip,
       *        in order (back to front), using the provided draw
       *        context.
       * @param strip The clipping rectangle.
       * @param ctx The draw context for rendering.
       */
      void DrawOverlays(
        Rectangle strip,
        const DrawContext& ctx
      );

    private:
      /**
       * @brief The overlay table.
       */
      Overlay _overlays[MaxOverlays] = {};

      /**
       * @brief Next overlay ID to assign.
       */
      UInt32 _nextID = 1;
  };
}
