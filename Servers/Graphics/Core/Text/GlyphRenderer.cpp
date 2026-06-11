/**
 * @file Servers/Graphics/Core/Text/GlyphRenderer.cpp
 * @brief Implements @ref DrawGlyph.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "GlyphRenderer.hpp"

#include "../Display/Display.hpp"

namespace Quantum::Servers::Graphics {
  void DrawGlyph(
    Display& display,
    UInt16 x, UInt16 y,
    UInt8 character,
    UInt32 foreground,
    UInt32 background,
    bool hasBackground,
    bool bold
  ) {
    for (UInt8 row = 0; row < display.font->Height; row++) {
      UInt8 bits = display.font->GetRow(character, row);

      if (bold) {
        bits |= (bits >> 1);
      }

      for (UInt8 col = 0; col < display.font->Width; col++) {
        bool set = (bits >> (7 - col)) & 1;

        UInt16 pixelX = static_cast<UInt16>(x + col);
        UInt16 pixelY = static_cast<UInt16>(y + row);

        if (set) {
          display.backBuffer.FillRectangle(
            pixelX, pixelY, 1, 1, foreground
          );
          display.driver->FillRectangle(
            pixelX, pixelY, 1, 1, foreground
          );
        } else if (hasBackground) {
          display.backBuffer.FillRectangle(
            pixelX, pixelY, 1, 1, background
          );
          display.driver->FillRectangle(
            pixelX, pixelY, 1, 1, background
          );
        }
      }
    }
  }
}
