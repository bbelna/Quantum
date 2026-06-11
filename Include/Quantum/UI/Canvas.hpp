/**
 * @file Include/Quantum/UI/Canvas.hpp
 * @brief Declares @ref @QUI::Canvas.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/UI/Drawable.hpp>
#include <Quantum/UI/Painter.hpp>

namespace Quantum::UI {
  /**
   * @brief A mutable view over an ARGB32 pixel buffer — the surface that
   *        painters render onto.
   * @note Does not own the underlying pixel memory.
   */
  class Canvas : public Drawable {
    public:
      /**
       * @brief Constructs a canvas over an existing pixel buffer.
       * @param pixels Pointer to the first pixel (ARGB32 format).
       * @param width Active width in pixels.
       * @param height Active height in pixels.
       * @param stride Row width in pixels (may exceed `width` for padding).
       */
      Canvas(UInt32* pixels, UInt16 width, UInt16 height, UInt16 stride);

      /**
       * @brief Constructs a canvas over a pixel buffer with an explicit
       *        bytes-per-pixel value.
       * @param pixels Pointer to the first pixel.
       * @param width Active width in pixels.
       * @param height Active height in pixels.
       * @param stride Row width in pixels (may exceed `width` for padding).
       * @param bytesPerPixel `2` for RGB565, `4` for ARGB32.
       */
      Canvas(
        void* pixels,
        UInt16 width,
        UInt16 height,
        UInt16 stride,
        UInt8 bytesPerPixel
      );

      /**
       * @brief Constructs an empty, invalid canvas.
       */
      Canvas();

      Canvas(const Canvas&) = delete;
      Canvas& operator=(const Canvas&) = delete;

      Canvas(Canvas&& other) noexcept;
      Canvas& operator=(Canvas&& other) noexcept;

      /**
       * @brief No-op — a canvas is inherently the drawing target.
       */
      void Draw() override {}

      /**
       * @brief Returns the full canvas area as a bounding rectangle.
       */
      Geometry2D::Rectangle GetBounds() const override {
        return Geometry2D::Rectangle(0, 0, _width, _height);
      }

      /**
       * @brief Returns the default @ref Painter for this canvas.
       */
      Painter& GetPainter() { return _painter; }

      /**
       * @brief Returns the default @ref Painter for this canvas (const).
       */
      const Painter& GetPainter() const { return _painter; }

      /**
       * @brief Updates the active dimensions (e.g. after a resize). The
       *        underlying pixel pointer and stride are unchanged.
       * @param width New active width in pixels.
       * @param height New active height in pixels.
       */
      void SetDimensions(UInt16 width, UInt16 height);

      /**
       * @brief Replaces the pixel buffer, dimensions, stride, and BPP
       *        in place without disturbing the painter's font.
       * @param pixels New pixel buffer.
       * @param width New active width.
       * @param height New active height.
       * @param stride New row stride.
       * @param bytesPerPixel New bytes per pixel.
       */
      void Reset(
        void* pixels,
        UInt16 width,
        UInt16 height,
        UInt16 stride,
        UInt8 bytesPerPixel
      );

      /**
       * @brief Converts an ARGB32 color to RGB565.
       */
      static UInt16 ToRGB565(UInt32 argb);

      /**
       * @brief Returns the active width in pixels.
       * @return The active width in pixels.
       */
      UInt16 GetWidth() const { return _width; }

      /**
       * @brief Returns the active height in pixels.
       * @return The active height in pixels.
       */
      UInt16 GetHeight() const { return _height; }

      /**
       * @brief Returns the row stride in pixels.
       * @note The stride may exceed the width due to padding.
       */
      UInt16 GetStride() const { return _stride; }

      /**
       * @brief Returns the raw pixel pointer.
       * @return Pointer to the first pixel in the buffer.
       */
      void* GetPixels() { return _pixels; }

      /**
       * @brief Returns the bytes per pixel.
       * @return `2` for RGB565, `4` for ARGB32.
       */
      UInt8 GetBytesPerPixel() const { return _bytesPerPixel; }

      /**
       * @brief Indicates whether the canvas has a valid pixel buffer.
       * @return `true` if the canvas has a non-null pixel buffer; `false`
       *         otherwise.
       */
      bool IsValid() const { return _pixels != nullptr; }

    private:
      /**
       * @brief Pointer to the first pixel in the buffer (ARGB32 format).
       */
      void* _pixels;

      /**
       * @brief Active width in pixels (may be less than stride for padding).
       */
      UInt16 _width;

      /**
       * @brief Active height in pixels.
       */
      UInt16 _height;

      /**
       * @brief Row stride in pixels (may exceed width for padding).
       */
      UInt16 _stride;

      /**
       * @brief Bytes per pixel (`2` for RGB565, `4` for ARGB32).
       */
      UInt8 _bytesPerPixel;

      /**
       * @brief The default painter for this canvas.
       */
      Painter _painter;
  };
}
