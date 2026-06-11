/**
 * @file Include/Quantum/HAL/Graphics/Payloads/ScreenBlitPayload.hpp
 * @brief Declaration of the ScreenBlitPayload structure.
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
   * @brief Payload for the `ScreenBlit` graphics driver operation.
   */
  struct ScreenBlitPayload {
    /**
     * @brief The x-coordinate of the source region's top-left corner.
     */
    UInt16 SourceX;

    /**
     * @brief The y-coordinate of the source region's top-left corner.
     */
    UInt16 SourceY;

    /**
     * @brief The x-coordinate of the destination region's top-left corner.
     */
    UInt16 DestinationX;

    /**
     * @brief The y-coordinate of the destination region's top-left corner.
     */
    UInt16 DestinationY;

    /**
     * @brief The width of the region in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the region in pixels.
     */
    UInt16 Height;
  };
}
