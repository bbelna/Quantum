/**
 * @file Include/Quantum/HAL/Graphics/Payloads/SetHardwareCursorBitmapPayload.hpp
 * @brief Declaration of the SetHardwareCursorBitmapPayload structure.
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
   * @brief Payload for the `SetHardwareCursorBitmap` operation.
   */
  struct SetHardwareCursorBitmapPayload {
    /**
     * @brief Width of the cursor bitmap in pixels (max 64).
     */
    UInt8 Width;

    /**
     * @brief Height of the cursor bitmap in pixels (max 64).
     */
    UInt8 Height;

    /**
     * @brief 32-bit ARGB color treated as transparent (not drawn).
     */
    UInt32 TransparentColor;

    /**
     * @brief Pointer to `Width * Height` 32-bit ARGB pixel values.
     */
    const UInt32* Pixels;
  };
}
