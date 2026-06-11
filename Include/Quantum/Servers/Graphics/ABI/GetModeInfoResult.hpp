/**
 * @file Include/Quantum/Servers/Graphics/ABI/GetModeInfoResult.hpp
 * @brief Declares @ref GraphicsGetModeInfoResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Result of a @ref GraphicsServerOperation::GetModeInfo request.
   */
  struct GraphicsGetModeInfoResult {
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

    /**
     * @brief `true` if the graphics driver provides a hardware cursor
     *        composited at the RAMDAC level, independent of the
     *        framebuffer. When `true`, callers need not hide the cursor
     *        around drawing operations.
     */
    bool HardwareCursor;

    /**
     * @brief The display scale factor (1 = standard density, 2 = HiDPI 2x,
     *        etc.). The Window Server divides the physical resolution by
     *        this value to obtain the logical resolution, and multiplies
     *        all outgoing coordinates by it before sending them to the
     *        Graphics Server.
     */
    UInt8 ScaleFactor;

    /**
     * @brief `true` if the graphics driver provides hardware-accelerated
     *        screen-to-screen BLT (e.g. S3 ViRGE 2D engine). When `true`,
     *        callers can use `ScreenBlit` for fast VRAM-to-VRAM copies
     *        instead of re-flushing the full region from the back buffer.
     */
    bool HasFastScreenBlit;
  };
}
