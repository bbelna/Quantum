/**
 * @file Include/Quantum/Servers/Graphics/ABI/BlitBufferRequest.hpp
 * @brief Declares @ref GraphicsBlitBufferRequest.
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
   * @brief Request to copy a pixel buffer to the framebuffer. The pixel
   *        data is stored inline immediately following the struct header.
   */
  struct GraphicsBlitBufferRequest : public GraphicsServerRequest {
    /**
     * @brief The x-coordinate of the top-left corner.
     */
    UInt16 X;

    /**
     * @brief The y-coordinate of the top-left corner.
     */
    UInt16 Y;

    /**
     * @brief The width of the rectangle in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the rectangle in pixels.
     */
    UInt16 Height;

    /**
     * @brief The color value that represents transparency. Pixels in the
     *        buffer matching this value will not be drawn.
     */
    UInt32 TransparentColor;

    /**
     * @brief Inline pixel data (variable length). Each pixel is a 32-bit
     *        ARGB value.
     */
    UInt32 Pixels[];
  };
}
