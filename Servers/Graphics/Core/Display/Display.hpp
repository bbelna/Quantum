/**
 * @file Servers/Graphics/Core/Display/Display.hpp
 * @brief Declares @ref @QGfxSrv::Display.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

#include "BackBuffer.hpp"
#include "../Cursor/Cursor.hpp"

namespace Quantum::Servers::Graphics {
  /**
   * @brief Owns a single physical display output.
   *
   * Holds the shared system-memory @ref BackBuffer, the RAII-owned
   * kernel @ref GraphicsDriverClient bridge, the active bitmap font, and
   * the current mode information (width, height, pitch, bit depth,
   * hardware-capability flags, framebuffer sharing state).
   *
   * The fanout draw methods (@ref FillRectangle, @ref BlitBuffer, etc.)
   * encapsulate the batch/cursor-overlap choreography that was
   * previously duplicated across every drawing controller handler:
   * each method writes to both the @ref BackBuffer and the driver,
   * and either expands the dirty rectangle (batch mode) or
   * temporarily undraws/redraws the software cursor (non-batch mode).
   *
   * @ref Display is non-copyable and non-movable: it owns the driver
   * pointer and would double-free on copy/move.
   */
  class Display {
    public:
      /**
       * @brief Creates a new @ref Display.
       * @param kernel
       *   Reference to the @ref KernelClient, forwarded to the owned
       *   @ref BackBuffer. Must outlive this instance.
       * @param log
       *   Reference to the @ref ServerLog, forwarded to the owned
       *   @ref BackBuffer. Must outlive this instance.
       * @param cursor
       *   Reference to the @ref Cursor. Used by draw operations to
       *   check overlap and temporarily undraw/redraw the cursor.
       *   Must outlive this instance.
       */
      Display(
        KernelClient& kernel,
        ServerLog& log,
        Cursor& cursor
      );

      /**
       * @brief Destroys the @ref Display and deletes the owned
       *        @ref GraphicsDriverClient, if any.
       */
      ~Display();

      Display(const Display&) = delete;
      Display(Display&&) = delete;
      Display& operator=(const Display&) = delete;
      Display& operator=(Display&&) = delete;

      // -----------------------------------------------------------------
      // Fanout draw operations
      // -----------------------------------------------------------------

      /**
       * @brief Fills a rectangle, writing to both the back buffer and
       *        the driver, with batch/cursor handling.
       */
      void FillRectangle(
        UInt16 x, UInt16 y,
        UInt16 width, UInt16 height,
        UInt32 color
      );

      /**
       * @brief Blits a pixel buffer, writing to both the back buffer
       *        and the driver, with batch/cursor handling.
       */
      void BlitBuffer(
        UInt16 x, UInt16 y,
        UInt16 width, UInt16 height,
        UInt32 transparent,
        const UInt32* pixels
      );

      /**
       * @brief XORs a rectangle, writing to both the back buffer and
       *        the driver, with batch/cursor handling.
       */
      void XORRectangle(
        UInt16 x, UInt16 y,
        UInt16 width, UInt16 height,
        UInt32 color
      );

      /**
       * @brief Renders pixel-based text using the active bitmap font,
       *        writing to both the back buffer and the driver, with
       *        batch/cursor handling.
       * @param x x-coordinate of the first glyph's top-left corner.
       * @param y y-coordinate of the first glyph's top-left corner.
       * @param text Pointer to the null-terminated string.
       * @param textLength Number of characters to render.
       * @param foreground 32-bit ARGB foreground color.
       * @param background 32-bit ARGB background color.
       * @param hasBackground If `false`, background pixels are skipped.
       * @param bold If `true`, each glyph row is widened by one pixel.
       */
      void DrawText(
        UInt16 x, UInt16 y,
        const char* text,
        Size textLength,
        UInt32 foreground,
        UInt32 background,
        bool hasBackground,
        bool bold
      );

      // -----------------------------------------------------------------
      // Batch control
      // -----------------------------------------------------------------

      /**
       * @brief Enters batch mode: subsequent drawing operations update
       *        only the shadow buffer, deferring VRAM writes.
       */
      void BeginBatch();

      /**
       * @brief Exits batch mode: flushes the accumulated dirty region
       *        to VRAM and redraws the cursor if it overlaps.
       */
      void EndBatch();

      /**
       * @brief Returns whether the display is currently in batch mode.
       * @return `true` if in batch mode.
       */
      bool IsInBatchMode() const {
        return _batchMode;
      }

      /**
       * @brief Expands the accumulated dirty rectangle to include the
       *        specified region.
       * @param x x-coordinate of the region's top-left corner.
       * @param y y-coordinate of the region's top-left corner.
       * @param width Width of the region in pixels.
       * @param height Height of the region in pixels.
       */
      void ExpandDirtyRectangle(
        UInt16 x, UInt16 y,
        UInt16 width, UInt16 height
      );

      // -----------------------------------------------------------------
      // Public state (transitional — will become private accessors)
      // -----------------------------------------------------------------

      /**
       * @brief The shared system-memory back buffer.
       */
      BackBuffer backBuffer;

      /**
       * @brief Pointer to the kernel graphics driver bridge.
       *
       * Populated by the server entry point after device discovery and
       * owned by this @ref Display: the destructor deletes it.
       */
      GraphicsDriverClient* driver = nullptr;

      /**
       * @brief The active bitmap font used for text rendering.
       */
      const BitmapFont* font = nullptr;

      /**
       * @brief The width of the display in pixels.
       */
      UInt16 screenWidth = 0;

      /**
       * @brief The height of the display in pixels.
       */
      UInt16 screenHeight = 0;

      /**
       * @brief The number of bits per pixel for the current video mode.
       */
      UInt8 bitsPerPixel = 0;

      /**
       * @brief The number of bytes per row in the framebuffer (pitch).
       */
      UInt16 pitch = 0;

      /**
       * @brief Whether the active graphics driver provides a hardware
       *        cursor overlaid at the RAMDAC level.
       */
      bool hasHardwareCursor = false;

      /**
       * @brief Whether the active graphics driver provides
       *        hardware-accelerated screen-to-screen BLT.
       */
      bool hasFastScreenBlit = false;

      /**
       * @brief The shared buffer ID for the VRAM framebuffer, or `0`
       *        if not available.
       */
      UInt32 framebufferBufferID = 0;

      /**
       * @brief Whether the framebuffer can be written directly
       *        (VRAM mapped + hardware cursor available).
       */
      bool hasDirectFramebuffer = false;

    private:
      Cursor& _cursor;

      bool _batchMode = false;

      UInt16 _dirtyX1 = 0;
      UInt16 _dirtyY1 = 0;
      UInt16 _dirtyX2 = 0;
      UInt16 _dirtyY2 = 0;

  };
}
