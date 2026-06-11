/**
 * @file Include/Quantum/Cursors/CursorImage.hpp
 * @brief Declares @ref @QCursors::CursorImage.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "CursorType.hpp"

namespace Quantum::Cursors {
  /**
   * @brief A non-owning view over a set of cursor bitmaps parsed from a
   *        QCUR file.
   *
   * Each cursor is stored as `Width * Height` contiguous 32-bit ARGB
   * pixels. Transparent pixels are `0x00000000`. The cursor index
   * within the file maps to a @ref CursorType enumerator.
   *
   * This is a plain data type with no virtual methods and no ownership
   * semantics.
   */
  struct CursorImage {
    /**
     * @brief Pointer to the first pixel of cursor 0.
     */
    const UInt32* PixelData = nullptr;

    /**
     * @brief Cursor width in pixels.
     */
    UInt8 Width = 0;

    /**
     * @brief Cursor height in pixels.
     */
    UInt8 Height = 0;

    /**
     * @brief Number of cursors in the set.
     */
    UInt8 CursorCount = 0;

    /**
     * @brief Per-cursor hotspot X coordinates, or `nullptr` if all
     *        cursors have hotspot (0, 0).
     */
    const UInt8* HotspotX = nullptr;

    /**
     * @brief Per-cursor hotspot Y coordinates, or `nullptr` if all
     *        cursors have hotspot (0, 0).
     */
    const UInt8* HotspotY = nullptr;

    /**
     * @brief Returns the pixel data for a cursor by type.
     * @param type The cursor type to retrieve.
     * @return Pointer to the first ARGB pixel, or `nullptr` if the
     *         cursor type is not present in the set.
     */
    const UInt32* GetCursor(CursorType type) const {
      UInt8 index = static_cast<UInt8>(type);

      if (!PixelData || index >= CursorCount) return nullptr;

      UInt32 pixelsPerCursor =
        static_cast<UInt32>(Width) * static_cast<UInt32>(Height);

      return PixelData + static_cast<UInt32>(index) * pixelsPerCursor;
    }

    /**
     * @brief Returns the hotspot X coordinate for a cursor by type.
     * @param type The cursor type.
     * @return Hotspot X, or `0` if not available.
     */
    UInt8 GetHotspotX(CursorType type) const {
      UInt8 index = static_cast<UInt8>(type);

      if (!HotspotX || index >= CursorCount) return 0;

      return HotspotX[index];
    }

    /**
     * @brief Returns the hotspot Y coordinate for a cursor by type.
     * @param type The cursor type.
     * @return Hotspot Y, or `0` if not available.
     */
    UInt8 GetHotspotY(CursorType type) const {
      UInt8 index = static_cast<UInt8>(type);

      if (!HotspotY || index >= CursorCount) return 0;

      return HotspotY[index];
    }
  };
}
