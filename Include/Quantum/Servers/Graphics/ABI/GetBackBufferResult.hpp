/**
 * @file Include/Quantum/Servers/Graphics/ABI/GetBackBufferResult.hpp
 * @brief Declares @ref GraphicsGetBackBufferResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Memory.hpp>

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Result of a @ref GraphicsServerOperation::GetBackBuffer
   *        request.
   */
  struct GraphicsGetBackBufferResult {
    /**
     * @brief The shared buffer ID for the ARGB32 back buffer.
     */
    SharedBufferID BufferID;

    /**
     * @brief The width of the back buffer in pixels.
     */
    UInt16 Width;

    /**
     * @brief The height of the back buffer in pixels.
     */
    UInt16 Height;

    /**
     * @brief The display scale factor (1 = standard density, 2 = HiDPI).
     */
    UInt8 ScaleFactor;

    /**
     * @brief `true` if the graphics driver provides a hardware cursor.
     */
    bool HardwareCursor;

    /**
     * @brief `true` if the graphics driver provides hardware-accelerated
     *        screen-to-screen BLT.
     */
    bool HasFastScreenBlit;

    /**
     * @brief The bits per pixel of the back buffer's native format.
     *        16 = RGB565, 32 = ARGB32.
     */
    UInt8 BitsPerPixel;

    /**
     * @brief `true` if the buffer points directly to VRAM. When set,
     *        FlushBackBuffer is unnecessary (writes go to the display
     *        immediately).
     */
    bool DirectFramebuffer;
  };
}
