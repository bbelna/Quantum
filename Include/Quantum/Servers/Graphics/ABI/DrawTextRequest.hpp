/**
 * @file Include/Quantum/Servers/Graphics/ABI/DrawTextRequest.hpp
 * @brief Declares @ref GraphicsDrawTextRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "../GraphicsServerRequest.hpp"

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Request to render pixel-based text using the server-side bitmap
   *        font. The text string is stored inline immediately following
   *        the struct.
   */
  struct GraphicsDrawTextRequest : public GraphicsServerRequest {
    /**
     * @brief x-coordinate of the top-left corner of the first glyph.
     */
    UInt16 X;

    /**
     * @brief y-coordinate of the top-left corner of the first glyph.
     */
    UInt16 Y;

    /**
     * @brief 32-bit ARGB foreground color.
     */
    UInt32 ForegroundColor;

    /**
     * @brief 32-bit ARGB background color. Alpha 0 (`0x00??????`) means
     *        transparent, background pixels are not drawn.
     */
    UInt32 BackgroundColor;

    /**
     * @brief Point-size hint for future font scaling. Currently ignored;
     *        text is always rendered at 8x14 pixels per glyph.
     */
    UInt16 FontSize;

    /**
     * @brief CSS-style font weight. Values above 500 select synthetic bold
     *        (each glyph row is widened by one pixel to the right).
     *        400 = normal, 700 = bold.
     */
    UInt16 FontWeight;

    /**
     * @brief Number of characters in @ref Text (not counting null
     *        terminator).
     */
    Size TextLength;

    /**
     * @brief Inline null-terminated string.
     */
    char Text[];
  };
}
