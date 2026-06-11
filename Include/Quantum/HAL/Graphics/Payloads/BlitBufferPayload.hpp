/**
 * @file Include/Quantum/HAL/Graphics/Payloads/BlitBufferPayload.hpp
 * @brief Declaration of the BlitBufferPayload structure.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::Graphics::Payloads {
  /**
   * @brief Payload for the `BlitBuffer` graphics driver operation.
   */
  struct BlitBufferPayload {
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
     * @brief Pointer to the pixel data buffer, which contains
     *        `Width` * `Height` pixels, each represented as a 32-bit ARGB
     *        value.
     */
    const UInt32* Pixels;
  };
}
