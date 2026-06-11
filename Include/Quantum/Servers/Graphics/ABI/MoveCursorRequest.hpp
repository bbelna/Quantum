/**
 * @file Include/Quantum/Servers/Graphics/ABI/MoveCursorRequest.hpp
 * @brief Declares @ref GraphicsMoveCursorRequest.
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
   * @brief Request to move the cursor to a new screen position.
   */
  struct GraphicsMoveCursorRequest : public GraphicsServerRequest {
    /**
     * @brief The new x-coordinate for the cursor.
     */
    Int16 X;

    /**
     * @brief The new y-coordinate for the cursor.
     */
    Int16 Y;

    /**
     * @brief When true, update cursor position in the shadow buffer only
     *        without flushing to VRAM. The caller is responsible for
     *        flushing the cursor region via a subsequent FlushBackBuffer.
     */
    bool SuppressFlush;
  };
}
