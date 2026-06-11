/**
 * @file Servers/Graphics/Core/Text/GlyphRenderer.hpp
 * @brief Declares @ref DrawGlyph.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

namespace Quantum::Servers::Graphics {
  class Display;

  /**
   * @brief Renders a single glyph from the active bitmap font into
   *        both the back buffer and the driver shadow.
   * @param display The @ref Display whose back buffer and driver
   *                receive the pixel writes.
   * @param x Physical pixel X of the glyph's top-left corner.
   * @param y Physical pixel Y of the glyph's top-left corner.
   * @param character Character code (indexes the active font).
   * @param foreground 32-bit ARGB foreground color.
   * @param background 32-bit ARGB background color.
   * @param hasBackground If `false`, background pixels are not drawn.
   * @param bold If `true`, each glyph row is widened by one pixel.
   */
  void DrawGlyph(
    Display& display,
    UInt16 x, UInt16 y,
    UInt8 character,
    UInt32 foreground,
    UInt32 background,
    bool hasBackground,
    bool bold
  );
}
