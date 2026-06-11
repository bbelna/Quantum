/**
 * @file Servers/App/Windows/WindowTitleBarButtonBitmaps.hpp
 * @brief AA coverage map and stamp helper for traffic-light title bar buttons.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

namespace Quantum::Servers::App::Windows::WindowTitleBarButtonBitmaps {
  /// @brief Button bitmap width in pixels.
  static constexpr UInt16 ButtonWidth = 12;

  /// @brief Button bitmap height in pixels.
  static constexpr UInt16 ButtonHeight = 12;

  /// @brief Total pixel count for a button bitmap.
  static constexpr UInt16 PixelCount = ButtonWidth * ButtonHeight;

  // ── AA coverage map ────────────────────────────────────────────────────
  //
  // 12x12 circle, center (6.0, 6.0), radius 6.0, ±0.5px transition zone.
  // 255 = fully inside, 0 = fully outside, intermediate = edge coverage.

  /// @brief Per-pixel coverage values for the 12x12 circle shape.
  static constexpr UInt8 Coverage[PixelCount] = {
      0,   0,   0, 117, 204, 249, 249, 204, 117,   0,   0,   0,
      0,  35, 204, 255, 255, 255, 255, 255, 255, 204,  35,   0,
      0, 204, 255, 255, 255, 255, 255, 255, 255, 255, 204,   0,
    117, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 117,
    204, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 204,
    249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249,
    249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249,
    204, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 204,
    117, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 117,
      0, 204, 255, 255, 255, 255, 255, 255, 255, 255, 204,   0,
      0,  35, 204, 255, 255, 255, 255, 255, 255, 204,  35,   0,
      0,   0,   0, 117, 204, 249, 249, 204, 117,   0,   0,   0,
  };

  /**
   * @brief Stamps a 12x12 circle bitmap into a caller-provided ARGB32
   *        buffer using the coverage map and the given fill color.
   * @param destination Pointer to a `PixelCount`-element ARGB32 array.
   * @param color The fill color (alpha channel is replaced by coverage).
   */
  static inline void Stamp(UInt32* destination, UInt32 color) {
    UInt32 rgb = color & 0x00FFFFFF;

    for (UInt16 i = 0; i < PixelCount; ++i) {
      UInt8 coverage = Coverage[i];

      destination[i] = (coverage == 0)
        ? 0x00000000
        : (static_cast<UInt32>(coverage) << 24) | rgb;
    }
  }
}
