/**
 * @file Include/Quantum/HAL/Graphics/Payloads/WriteNativeRegionPayload.hpp
 * @brief Declaration of the WriteNativeRegionPayload structure.
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
   * @brief Payload for the `WriteNativeRegion` driver operation.
   */
  struct WriteNativeRegionPayload {
    /**
     * @brief The x-coordinate of the destination top-left corner.
     */
    UInt16 X;

    /**
     * @brief The y-coordinate of the destination top-left corner.
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

    /**
     * @brief The width of the source buffer in pixels (row stride).
     */
    UInt32 SrcPitch;

    /**
     * @brief Pointer to the native-format pixel data. The region starts at
     *        byte offset `Y * SrcPitch * bytesPerPixel + X * bytesPerPixel`.
     */
    const void* Pixels;
  };
}
