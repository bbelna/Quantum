/**
 * @file Include/Quantum/HAL/Graphics/Payloads/ModeInfoPayload.hpp
 * @brief Declaration of the ModeInfoPayload structure.
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
   * @brief Payload for the `GetModeInfo` graphics driver operation. This is
   *        an in-out payload: the caller allocates the struct and passes a
   *        pointer; the driver fills in the fields.
   */
  struct ModeInfoPayload {
    /**
     * @brief The width of the display in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the display in pixels.
     */
    UInt16 Height;

    /**
     * @brief The number of bits per pixel (e.g., 4, 8, 16, 24, 32).
     */
    UInt8 BitsPerPixel;

    /**
     * @brief The number of bytes per row in the framebuffer (pitch).
     */
    UInt16 Pitch;
  };
}
