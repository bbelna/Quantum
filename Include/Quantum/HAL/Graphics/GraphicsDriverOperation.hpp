/**
 * @file Include/Quantum/HAL/Graphics/GraphicsDriverOperation.hpp
 * @brief Declares @ref Quantum::HAL::Graphics::GraphicsDriverOperation.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::HAL::Graphics {
  /**
   * @brief Operation codes for graphics drivers.
   */
  enum class GraphicsDriverOperation : UInt32 {
    /**
     * @brief Writes text to the screen at the current text cursor position.
     */
    WriteText = 1,

    /**
     * @brief Switches the display to a specified video mode.
     */
    SetMode = 2,

    /**
     * @brief Fills a rectangle with a solid color.
     */
    FillRectangle = 3,

    /**
     * @brief Copies a pixel buffer to the framebuffer.
     */
    BlitBuffer = 4,

    /**
     * @brief XORs a rectangle with a color.
     */
    XORRectangle = 5,

    /**
     * @brief Queries the current video mode information.
     */
    GetModeInfo = 6,

    /**
     * @brief Enables or disables batch mode. In batch mode, drawing operations
     *        update the shadow buffer only and skip the framebuffer copy.
     */
    SetBatchMode = 7,

    /**
     * @brief Flushes a region from the shadow buffer to the framebuffer.
     */
    FlushRegion = 8,

    /**
     * @brief Moves a rectangular region of the screen to a new position
     *        using screen-to-screen copy (BITBLT SRCCOPY).
     */
    ScreenBlit = 9,

    /**
     * @brief Queries whether the driver supports a hardware cursor overlaid
     *        at the RAMDAC level (no VRAM compositing needed).
     */
    GetHardwareCursorSupport = 10,

    /**
     * @brief Uploads an ARGB cursor bitmap to the hardware cursor and sets
     *        its colors. Must be called before enabling the cursor.
     */
    SetHardwareCursorBitmap = 11,

    /**
     * @brief Moves the hardware cursor to the specified screen coordinates.
     */
    SetHardwareCursorPosition = 12,

    /**
     * @brief Shows or hides the hardware cursor.
     */
    SetHardwareCursorVisible = 13,

    /**
     * @brief Writes a rectangular region of ARGB32 pixels to the shadow
     *        buffer, converting to the native pixel format. The source
     *        buffer has a specified pitch (row width in pixels) so that
     *        sub-regions of a larger buffer can be written efficiently.
     */
    WritePixelRegion = 14,

    /**
     * @brief Queries whether the driver provides hardware-accelerated
     *        screen-to-screen BLT (VRAM-to-VRAM copy via a 2D engine).
     */
    GetFastScreenBlitSupport = 15,

    /**
     * @brief Writes a rectangular region of native-format pixels to the
     *        shadow buffer and framebuffer without any format conversion.
     *        For 16bpp modes the source is RGB565; for 32bpp it is ARGB32.
     */
    WriteNativeRegion = 16,

    /**
     * @brief Returns the SharedBufferID of the VRAM framebuffer, or 0 if
     *        direct VRAM access is not available.
     */
    GetFramebufferBufferID = 17,

    /**
     * @brief Acquires exclusive display ownership for a process, switching
     *        the driver to compositing mode.
     */
    AcquireDisplay = 18,

    /**
     * @brief Releases display ownership, reverting the driver to text mode.
     */
    ReleaseDisplay = 19,

    /**
     * @brief Queries the current display owner and compositing state.
     */
    GetDisplayOwner = 20
  };
}
