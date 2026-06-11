/**
 * @file Include/Quantum/Servers/Graphics/ABI/SetCursorBitmapRequest.hpp
 * @brief Declares @ref GraphicsSetCursorBitmapRequest.
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
   * @brief Request to set the cursor bitmap. The pixel data is stored
   *        inline immediately following the struct header.
   */
  struct GraphicsSetCursorBitmapRequest : public GraphicsServerRequest {
    /**
     * @brief The width of the cursor bitmap in pixels.
     */
    UInt8 Width;

    /**
     * @brief The height of the cursor bitmap in pixels.
     */
    UInt8 Height;

    /**
     * @brief The color value that represents transparency. Pixels in the
     *        bitmap matching this value will not be drawn.
     */
    UInt32 TransparentColor;

    /**
     * @brief Inline pixel data (variable length). Each pixel is a 32-bit
     *        ARGB value.
     */
    UInt32 Pixels[];
  };
}
