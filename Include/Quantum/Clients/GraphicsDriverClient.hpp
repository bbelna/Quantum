/**
 * @file Include/Quantum/Clients/GraphicsDriverClient.hpp
 * @brief Declares @ref @QClients::GraphicsDriverClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/HAL/Graphics.hpp>

#include "DriverClient.hpp"

namespace Quantum::Clients {
  /**
   * @brief Client-side interface for graphics driver operations.
   *
   * Extends @ref DriverClient with typed wrapper methods for all
   * graphics driver operations. The caller does not need to know
   * whether the driver is a kernel-mode or userspace driver.
   *
   * @code
   *   GraphicsDriverClient driver;
   *   driver.Initialize(graphicsDevice.ID);
   *
   *   auto modeInfo = driver.QueryModeInfo();
   *   driver.FillRectangle(0, 0, modeInfo.Width, modeInfo.Height, 0xFF000000);
   * @endcode
   */
  class GraphicsDriverClient : public DriverClient {
    public:

      // ----- Query operations -----

      /**
       * @brief Queries the current video mode information.
       * @return A @ref HAL::Graphics::Payloads::ModeInfoPayload with the
       *         current resolution, bit depth, and pitch.
       */
      HAL::Graphics::Payloads::ModeInfoPayload QueryModeInfo();

      /**
       * @brief Queries whether the driver supports a hardware cursor.
       * @return `true` if a hardware cursor is available.
       */
      bool HasHardwareCursor();

      /**
       * @brief Queries whether the driver supports hardware-accelerated
       *        screen-to-screen BLT.
       * @return `true` if fast VRAM-to-VRAM BLT is available.
       */
      bool HasFastScreenBlit();

      /**
       * @brief Queries the VRAM framebuffer SharedBufferID.
       * @return The SharedBufferID, or 0 if not available.
       */
      UInt32 GetFramebufferBufferID();

      // ----- Drawing operations -----

      /**
       * @brief Writes text at the current text cursor position.
       * @param text The null-terminated text to write.
       */
      void WriteText(char* text);

      /**
       * @brief Switches the display to a specified video mode.
       * @param mode The video mode number.
       */
      void SetMode(UInt16 mode);

      /**
       * @brief Fills a rectangle with a solid color.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param width The width in pixels.
       * @param height The height in pixels.
       * @param color The 32-bit ARGB color value.
       */
      void FillRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 color
      );

      /**
       * @brief Copies a pixel buffer to the framebuffer.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param width The width in pixels.
       * @param height The height in pixels.
       * @param transparent The color value that represents transparency.
       * @param pixels The ARGB32 pixel data buffer.
       */
      void BlitBuffer(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 transparent,
        const UInt32* pixels
      );

      /**
       * @brief XORs a rectangle with a color.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param width The width in pixels.
       * @param height The height in pixels.
       * @param color The 32-bit ARGB color value to XOR with.
       */
      void XORRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 color
      );

      /**
       * @brief Moves a rectangular region of the screen to a new position.
       * @param sourceX Source x-coordinate.
       * @param sourceY Source y-coordinate.
       * @param destinationX Destination x-coordinate.
       * @param destinationY Destination y-coordinate.
       * @param width Width of the region in pixels.
       * @param height Height of the region in pixels.
       */
      void ScreenBlit(
        UInt16 sourceX,
        UInt16 sourceY,
        UInt16 destinationX,
        UInt16 destinationY,
        UInt16 width,
        UInt16 height
      );

      /**
       * @brief Writes ARGB32 pixels to the shadow buffer with format
       *        conversion.
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param width Width of the region in pixels.
       * @param height Height of the region in pixels.
       * @param sourcePitch Row width of the source buffer in pixels.
       * @param pixels Pointer to the ARGB32 source buffer.
       */
      void WritePixelRegion(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 sourcePitch,
        const UInt32* pixels
      );

      /**
       * @brief Writes native-format pixels to the shadow buffer without
       *        format conversion.
       * @param x Destination x-coordinate.
       * @param y Destination y-coordinate.
       * @param width Width of the region in pixels.
       * @param height Height of the region in pixels.
       * @param sourcePitch Row width of the source buffer in pixels.
       * @param pixels Pointer to the native-format source buffer.
       */
      void WriteNativeRegion(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 sourcePitch,
        const void* pixels
      );

      // ----- Batch operations -----

      /**
       * @brief Enables or disables batch mode on the driver.
       * @param enabled `true` to enable, `false` to disable.
       */
      void SetBatchMode(bool enabled);

      /**
       * @brief Flushes a region from the shadow buffer to the framebuffer.
       * @param x The x-coordinate of the region's top-left corner.
       * @param y The y-coordinate of the region's top-left corner.
       * @param width The width of the region in pixels.
       * @param height The height of the region in pixels.
       */
      void FlushRegion(UInt16 x, UInt16 y, UInt16 width, UInt16 height);

      // ----- Cursor operations -----

      /**
       * @brief Uploads an ARGB cursor bitmap to the hardware cursor.
       * @param width Cursor width in pixels.
       * @param height Cursor height in pixels.
       * @param transparent Transparent color value.
       * @param pixels ARGB pixel data.
       */
      void SetHardwareCursorBitmap(
        UInt8 width,
        UInt8 height,
        UInt32 transparent,
        const UInt32* pixels
      );

      /**
       * @brief Moves the hardware cursor to the specified screen position.
       * @param x New x-coordinate.
       * @param y New y-coordinate.
       */
      void SetHardwareCursorPosition(Int16 x, Int16 y);

      /**
       * @brief Shows or hides the hardware cursor.
       * @param visible `true` to show, `false` to hide.
       */
      void SetHardwareCursorVisible(bool visible);

  };
}
