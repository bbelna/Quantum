/**
 * @file Include/Quantum/Fonts/DefaultFont.hpp
 * @brief Declares @ref @QFont::DefaultFont.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "BitmapFont.hpp"

#include <Quantum/Bitmaps.hpp>

namespace Quantum::Fonts {
  /**
   * @brief The default system font (`lat9w 8x12`).
   *
   * Wraps @ref @Q::Bitmaps::Font8x14 from @ref @Q::Bitmaps in a
   * @ref BitmapFont view.
   */
  static constexpr BitmapFont DefaultFont = {
    .GlyphData = &Bitmaps::Font8x14[0][0],
    .Width = Bitmaps::FontGlyphWidth,
    .Height = Bitmaps::FontGlyphHeight,
    .BytesPerGlyph = Bitmaps::FontGlyphHeight,
    .GlyphCount = Bitmaps::FontGlyphCount,
    .Advances = nullptr,
    .Ascent = 0,
    .BitsPerPixel = 1,
  };
}
