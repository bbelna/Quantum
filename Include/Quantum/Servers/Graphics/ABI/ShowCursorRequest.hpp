/**
 * @file Include/Quantum/Servers/Graphics/ABI/ShowCursorRequest.hpp
 * @brief Declares @ref GraphicsShowCursorRequest.
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
   * @brief Request to show or hide the cursor.
   */
  struct GraphicsShowCursorRequest : public GraphicsServerRequest {
    /**
     * @brief `true` to show the cursor, false to hide it.
     */
    bool Visible;
  };
}
