/**
 * @file Servers/Graphics/BackBuffer.hpp
 * @brief Declares @ref @QGfxSrv::BackBuffer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

namespace Quantum::Servers::Graphics {
  /**
   * @brief Manages the back buffer.
   * @note For 16bpp modes the buffer stores RGB565 pixels; for 32bpp modes it
   *       stores ARGB32.
   *
   * The back buffer stores the clean scene (without the cursor) and is shared
   * with the app server for direct compositing.
   */
  class BackBuffer {
    public:
      /**
       * @brief Creates a new @ref BackBuffer.
       * @param kernel
       *   Reference to a @ref KernelClient.
       *   Must outlive this instance.
       * @param log
       *   Reference to a @ref ServerLog.
       *   Must outlive this instance.
       */
      BackBuffer(
        KernelClient& kernel,
        ServerLog& log
      );

      /**
       * @brief Allocates the shared back buffer for the given display
       *        dimensions and bit depth.
       * @param width The width of the display in pixels.
       * @param height The height of the display in pixels.
       * @param bitsPerPixel The number of bits per pixel.
       * @return `true` if the allocation succeeded, `false` on failure.
       */
      bool Allocate(
        UInt16 width,
        UInt16 height,
        UInt8 bitsPerPixel
      );

      /**
       * @brief Fills a rectangle with a solid color.
       * @param x The \f$x\f$-coordinate of the top-left corner.
       * @param y The \f$y\f$-coordinate of the top-left corner.
       * @param width The width of the rectangle in pixels.
       * @param height The height of the rectangle in pixels.
       * @param color The color to fill with as an ARGB32 value.
       */
      void FillRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 color
      );

      /**
       * @brief Copies a pixel buffer to the back buffer, skipping
       *        transparent pixels.
       * @param x The \f$x\f$-coordinate of the top-left corner.
       * @param y The \f$y\f$-coordinate of the top-left corner.
       * @param width The width of the region in pixels.
       * @param height The height of the region in pixels.
       * @param transparent The ARGB32 color value that represents
       *                    transparency.
       * @param pixels The pixel data buffer containing 32-bit ARGB
       *               values.
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
       * @param x The \f$x\f$-coordinate of the top-left corner.
       * @param y The \f$y\f$-coordinate of the top-left corner.
       * @param width The width of the rectangle in pixels.
       * @param height The height of the rectangle in pixels.
       * @param color The color to XOR with as an ARGB32 value.
       */
      void XORRectangle(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height,
        UInt32 color
      );

      /**
       * @brief Copies a rectangular region to a new position within
       *        the back buffer.
       * @param sourceX Source \f$x\f$-coordinate.
       * @param sourceY Source \f$y\f$-coordinate.
       * @param destinationX Destination \f$x\f$-coordinate.
       * @param destinationY Destination \f$y\f$-coordinate.
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
       * @brief Returns an opaque `void` pointer to the raw back buffer memory.
       * @return Opaque `void` pointer to the buffer, or `nullptr` if not
       *         allocated.
       */
      void* GetBuffer() const {
        return _buffer;
      }

      /**
       * @brief Returns the @ref SharedBufferID.
       * @return The @ref SharedBufferID.
       */
      SharedBufferID GetBufferID() const {
        return _bufferID;
      }

      /**
       * @brief Returns the width of the back buffer in pixels.
       * @return The width in pixels.
       */
      UInt16 GetWidth() const {
        return _width;
      }

      /**
       * @brief Returns the height of the back buffer in pixels.
       * @return The height in pixels.
       */
      UInt16 GetHeight() const {
        return _height;
      }

      /**
       * @brief Returns the number of bytes per pixel.
       * @return `2` for 16bpp, `4` for 32bpp.
       */
      UInt8 GetBytesPerPixel() const {
        return BytesPerPixel(_format);
      }

      /**
       * @brief Returns the @ref PixelFormat describing the channel
       *        layout and bit depth of each pixel.
       * @return The @ref PixelFormat of the back buffer.
       */
      PixelFormat GetFormat() const {
        return _format;
      }

    private:
      /**
       * @brief Reference to a @ref KernelClient.
       */
      KernelClient& _kernel;

      /**
       * @brief Reference to a @ref ServerLog.
       */
      ServerLog& _log;

      /**
       * @brief Opaque `void` pointer to the shared back buffer memory.
       */
      void* _buffer = nullptr;

      /**
       * @brief The @ref SharedBufferID for cross-process mapping.
       */
      SharedBufferID _bufferID = 0;

      /**
       * @brief The width of the back buffer in pixels.
       */
      UInt16 _width = 0;

      /**
       * @brief The height of the back buffer in pixels.
       */
      UInt16 _height = 0;

      /**
       * @brief The @ref PixelFormat of each pixel in the back buffer.
       */
      PixelFormat _format = PixelFormat::ARGB32;
  };
}
