/**
 * @file Include/Quantum/Servers/Graphics/GraphicsServerOperation.hpp
 * @brief Declares @ref GraphicsServerOperation.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::Graphics {
  /**
   * @brief Operations supported by the graphics server.
   */
  enum class GraphicsServerOperation : UInt32 {
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
     * @brief Sets the cursor bitmap. The graphics server manages cursor
     *        compositing internally using the system-memory back buffer.
     */
    SetCursorBitmap = 6,

    /**
     * @brief Moves the cursor to a new screen position. The graphics server
     *        composites the cursor from the back buffer and blits to the
     *        framebuffer in a single operation.
     */
    MoveCursor = 7,

    /**
     * @brief Shows or hides the cursor.
     */
    ShowCursor = 8,

    /**
     * @brief Queries the current video mode information, including
     *        resolution and bits per pixel.
     */
    GetModeInfo = 9,

    /**
     * @brief Moves a rectangular region of the screen to a new position
     *        using screen-to-screen bit blit.
     */
    ScreenBlit = 10,

    /**
     * @brief Enters batch mode: subsequent drawing operations update the
     *        shadow buffer only, deferring VRAM writes until `EndBatch`.
     */
    BeginBatch = 11,

    /**
     * @brief Exits batch mode and flushes the accumulated dirty region
     *        from the shadow buffer to VRAM in a single operation.
     */
    EndBatch = 12,

    /**
     * @brief Renders a null-terminated string of characters at the specified
     *        pixel position using the embedded 8x14 bitmap font. `FontSize`
     *        is accepted but currently ignored; all text is rendered at 8x14.
     */
    DrawText = 13,

    /**
     * @brief Returns the shared buffer ID and dimensions of the graphics
     *        server's ARGB32 back buffer, so that the caller can map it
     *        via `Memory::AttachShared` and composite directly.
     */
    GetBackBuffer = 14,

    /**
     * @brief Pushes a rectangular region of the shared back buffer to the
     *        display. The graphics server converts ARGB32 pixels to the
     *        native format and flushes them to VRAM.
     */
    FlushBackBuffer = 15
  };
}

namespace Quantum::Servers::Graphics::ABI {
  /**
   * @brief Back-compat alias for @ref Graphics::GraphicsServerOperation.
   *
   * Kept so legacy call sites that reference
   * @c Quantum::Servers::Graphics::ABI::GraphicsOperation continue to
   * compile during the refactor. Prefer @ref GraphicsServerOperation.
   */
  using GraphicsOperation = Graphics::GraphicsServerOperation;
}
