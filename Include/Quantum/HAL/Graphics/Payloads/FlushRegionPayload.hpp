/**
 * @file Include/Quantum/HAL/Graphics/Payloads/FlushRegionPayload.hpp
 * @brief Declaration of the FlushRegionPayload structure.
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
   * @brief Payload for the `FlushRegion` graphics driver operation.
   */
  struct FlushRegionPayload {
    /**
     * @brief The x-coordinate of the region's top-left corner.
     */
    UInt16 X;

    /**
     * @brief The y-coordinate of the region's top-left corner.
     */
    UInt16 Y;

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
