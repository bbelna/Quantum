/**
 * @file Include/Quantum/Servers/Graphics/ABI/FlushBackBufferRequest.hpp
 * @brief Declares @ref GraphicsFlushBackBufferRequest.
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
   * @brief Request to flush a rectangular region of the shared back
   *        buffer to the display.
   */
  struct GraphicsFlushBackBufferRequest : public GraphicsServerRequestWithReply {
    /**
     * @brief The x-coordinate of the dirty region's top-left corner.
     */
    UInt16 X;

    /**
     * @brief The y-coordinate of the dirty region's top-left corner.
     */
    UInt16 Y;

    /**
     * @brief The width of the dirty region in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the dirty region in pixels.
     */
    UInt16 Height;
  };
}
