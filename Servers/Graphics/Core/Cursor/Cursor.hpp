/**
 * @file Servers/Graphics/Core/Cursor/Cursor.hpp
 * @brief Declares @ref @QGfxSrv::Cursor.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <GraphicsServerTypes.hpp>

namespace Quantum::Servers::Graphics {
  class BackBuffer;
  class Display;

  /**
   * @brief The software cursor: bitmap, position, visibility, and
   *        compositing.
   *
   * Owns the cursor sprite's pixel data (stored in a fixed-size
   * @ref MaxCursorWidth x @ref MaxCursorHeight buffer) along with its
   * current screen position, visibility flag, and transparent-color
   * key. @ref Draw composites the cursor over the current back buffer
   * contents and blits the result to the framebuffer in a single
   * driver call; @ref Undraw restores the clean back buffer contents
   * beneath the cursor's previous footprint.
   */
  class Cursor {
    public:
      /**
       * @brief A clipped, on-screen cursor rectangle.
       *
       * Produced by @ref ComputeVisibleRect. When
       * @ref VisibleWidth or @ref VisibleHeight is `0` the cursor
       * has no on-screen footprint (hidden, empty, or fully clipped
       * off-screen).
       */
      struct VisibleRect {
        /**
         * @brief The clipped top-left x-coordinate in screen space.
         */
        UInt16 X;

        /**
         * @brief The clipped top-left y-coordinate in screen space.
         */
        UInt16 Y;

        /**
         * @brief The visible width in pixels after clipping.
         */
        UInt8 VisibleWidth;

        /**
         * @brief The visible height in pixels after clipping.
         */
        UInt8 VisibleHeight;
      };

      /**
       * @brief Creates a hidden @ref Cursor with no bitmap and no
       *        position.
       */
      Cursor();

      /**
       * @brief Replaces the cursor bitmap and transparent color.
       * @param width The width of the new bitmap in pixels. Clamped to
       *              @ref MaxCursorWidth.
       * @param height The height of the new bitmap in pixels. Clamped to
       *               @ref MaxCursorHeight.
       * @param transparentColor The ARGB32 color value that represents
       *                         transparency in the bitmap.
       * @param pixels Pointer to a contiguous
       *               @p width x @p height ARGB32 pixel buffer, laid out
       *               in row-major order.
       */
      void SetBitmap(
        UInt8 width,
        UInt8 height,
        UInt32 transparentColor,
        const UInt32* pixels
      );

      /**
       * @brief Returns whether a bitmap has been set on this cursor.
       * @return `true` if @ref SetBitmap has been called with a
       *         non-empty bitmap, `false` otherwise.
       */
      bool HasBitmap() const {
        return _width > 0 && _height > 0;
      }

      /**
       * @brief Sets the cursor position in screen space.
       * @param x The new x-coordinate. May be negative to indicate
       *          that the cursor extends off the left edge.
       * @param y The new y-coordinate. May be negative to indicate
       *          that the cursor extends off the top edge.
       */
      void SetPosition(Int16 x, Int16 y) {
        _x = x;
        _y = y;
      }

      /**
       * @brief Returns the current x-coordinate of the cursor.
       * @return The current x-coordinate.
       */
      Int16 GetX() const {
        return _x;
      }

      /**
       * @brief Returns the current y-coordinate of the cursor.
       * @return The current y-coordinate.
       */
      Int16 GetY() const {
        return _y;
      }

      /**
       * @brief Sets whether the cursor is currently visible.
       * @param visible `true` to mark the cursor visible, `false` to
       *                mark it hidden.
       */
      void SetVisible(bool visible) {
        _visible = visible;
      }

      /**
       * @brief Returns whether the cursor is currently visible.
       * @return `true` if visible, `false` otherwise.
       */
      bool IsVisible() const {
        return _visible;
      }

      /**
       * @brief Returns the width of the current cursor bitmap.
       * @return The width in pixels, or `0` if no bitmap is set.
       */
      UInt8 GetWidth() const {
        return _width;
      }

      /**
       * @brief Returns the height of the current cursor bitmap.
       * @return The height in pixels, or `0` if no bitmap is set.
       */
      UInt8 GetHeight() const {
        return _height;
      }

      /**
       * @brief Returns the ARGB32 color value that represents
       *        transparency in the bitmap.
       * @return The transparent color.
       */
      UInt32 GetTransparentColor() const {
        return _transparentColor;
      }

      /**
       * @brief Composites the cursor bitmap over the back buffer and
       *        blits the result to the framebuffer in a single call.
       * @param display The @ref Display whose back buffer supplies the
       *                background pixels and whose driver receives the
       *                composited region.
       *
       * No-op when the cursor is hidden, has no bitmap, or is fully
       * clipped off-screen.
       */
      void Draw(Display& display);

      /**
       * @brief Restores the clean back buffer contents to the
       *        framebuffer in the region occupied by the cursor.
       * @param display The @ref Display whose back buffer supplies the
       *                clean background pixels and whose driver
       *                receives the restored region.
       *
       * No-op when the cursor is hidden, has no bitmap, or is fully
       * clipped off-screen.
       */
      void Undraw(Display& display);

      /**
       * @brief Tests whether the cursor's bounding box overlaps a
       *        rectangular region on the screen.
       * @param x The x-coordinate of the rectangle's top-left corner.
       * @param y The y-coordinate of the rectangle's top-left corner.
       * @param width The width of the rectangle in pixels.
       * @param height The height of the rectangle in pixels.
       * @return `true` if the cursor overlaps the rectangle, `false`
       *         otherwise.
       */
      bool Overlaps(
        UInt16 x,
        UInt16 y,
        UInt16 width,
        UInt16 height
      ) const;

      /**
       * @brief Computes the on-screen clipped rectangle for the
       *        current cursor position and bitmap dimensions.
       * @param screenWidth The screen width in pixels, used for
       *                    clipping.
       * @param screenHeight The screen height in pixels, used for
       *                     clipping.
       * @return The clipped @ref VisibleRect. When
       *         @ref VisibleRect::VisibleWidth or
       *         @ref VisibleRect::VisibleHeight is `0`, the cursor
       *         has no on-screen footprint.
       */
      VisibleRect ComputeVisibleRect(
        UInt16 screenWidth,
        UInt16 screenHeight
      ) const;

    private:
      /**
       * @brief The cursor bitmap, storing 32-bit ARGB values for each
       *        pixel of the cursor sprite. Only the top-left
       *        @ref _width x @ref _height region is meaningful.
       */
      UInt32 _bitmap[MaxCursorHeight][MaxCursorWidth] = {};

      /**
       * @brief The width of the current cursor bitmap in pixels.
       */
      UInt8 _width = 0;

      /**
       * @brief The height of the current cursor bitmap in pixels.
       */
      UInt8 _height = 0;

      /**
       * @brief The ARGB32 color value that represents transparency in
       *        the cursor bitmap.
       */
      UInt32 _transparentColor = 0xFFFFFFFF;

      /**
       * @brief The current x-coordinate of the cursor.
       */
      Int16 _x = 0;

      /**
       * @brief The current y-coordinate of the cursor.
       */
      Int16 _y = 0;

      /**
       * @brief Whether the cursor is currently visible.
       */
      bool _visible = false;
  };
}
