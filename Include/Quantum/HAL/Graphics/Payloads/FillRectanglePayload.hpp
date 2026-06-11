/**
 * @file Include/Quantum/HAL/Graphics/Payloads/FillRectanglePayload.hpp
 * @brief Declaration of the FillRectanglePayload structure.
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
   * @brief Payload for the `FillRectangle` graphics driver operation.
   */
  struct FillRectanglePayload {
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
     * @brief The color to fill the rectangle with. In palette modes, the low
     *        byte is the palette index. In true-color modes, the full 32-bit
     *        ARGB value is used.
     */
    UInt32 Color;
  };
}
