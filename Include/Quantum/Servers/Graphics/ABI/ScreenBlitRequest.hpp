/**
 * @file Include/Quantum/Servers/Graphics/ABI/ScreenBlitRequest.hpp
 * @brief Declares @ref GraphicsScreenBlitRequest.
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
   * @brief Request to move a rectangular screen region to a new position.
   */
  struct GraphicsScreenBlitRequest : public GraphicsServerRequestWithReply {
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
