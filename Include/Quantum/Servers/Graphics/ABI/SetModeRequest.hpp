/**
 * @file Include/Quantum/Servers/Graphics/ABI/SetModeRequest.hpp
 * @brief Declares @ref GraphicsSetModeRequest.
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
   * @brief Request to switch the display to a video mode.
   */
  struct GraphicsSetModeRequest : public GraphicsServerRequest {
    /**
     * @brief The video mode number (e.g., `0x12` for 640x480x16, `0x13`
     *        for 320x200x256, or a VESA mode number such as `0x101`).
     */
    UInt16 Mode;
  };
}
