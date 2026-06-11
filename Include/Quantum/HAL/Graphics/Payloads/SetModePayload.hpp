/**
 * @file Include/Quantum/HAL/Graphics/Payloads/SetModePayload.hpp
 * @brief Declaration of the SetModePayload structure.
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
   * @brief Payload for the `SetMode` graphics driver operation.
   */
  struct SetModePayload {
    /**
     * @brief The video mode number (e.g., 0x12 for 640x480x16, 0x13 for
     *        320x200x256, or a VESA mode number such as 0x101 for
     *        640x480x256).
     */
    UInt16 Mode;
  };
}
