/**
 * @file Servers/App/Core/DrawContext.hpp
 * @brief Declares and implements @ref Quantum::Servers::App::DrawKernel::Instance().Context()->
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Geometry2D.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::App {
  /**
   * @brief Lightweight draw context abstraction.
   */
  struct DrawContext {
    /**
     * @brief Function pointer type for filling a rectangle with a solid color.
     * @param userData Opaque pointer passed through from `UserData`.
     * @param rect The rectangle to fill.
     * @param color The 32-bit ARGB color value.
     */
    using FillFn = void (*)(
      void* userData,
      Geometry2D::Rectangle rect,
      UInt32 color
    );

    /**
     * @brief Function pointer type for rendering a string of text.
     * @param userData Opaque pointer passed through from `UserData`.
     * @param origin The top-left pixel coordinate of the first glyph.
     * @param text Pointer to the text to render.
     * @param length Number of characters to render.
     * @param color The 32-bit ARGB foreground color.
     * @param fontWeight CSS-style font weight (400 = normal, 700 = bold).
     *                   Values above 500 select the bold font.
     */
    using TextFn = void (*)(
      void* userData,
      Geometry2D::Point origin,
      const char* text,
      Size length,
      UInt32 color,
      UInt16 fontWeight
    );

    /**
     * @brief Function pointer type for blitting a pixel buffer into the
     *        compositing surface.
     * @param userData Opaque pointer passed through from `UserData`.
     * @param dstX Destination x-coordinate in the compositing buffer.
     * @param dstY Destination y-coordinate in the compositing buffer.
     * @param src Source pixel data.
     * @param w Width of the source region in pixels.
     * @param h Height of the source region in pixels.
     * @param srcStride Row stride of the source buffer in pixels.
     * @param srcBpp Bytes per pixel of the source buffer (2 or 4).
     */
    using BlitFn = void (*)(
      void* userData,
      Int16 dstX, Int16 dstY,
      const void* src,
      UInt16 w, UInt16 h,
      UInt16 srcStride,
      UInt8 srcBpp
    );

    /**
     * @brief Fills a rectangle with a solid ARGB32 color.
     */
    FillFn FillRectangle;

    /**
     * @brief Renders a string of 8×14 bitmap font glyphs.
     */
    TextFn RenderText;

    /**
     * @brief Blits a rectangular region of ARGB32 pixels into the
     *        compositing surface.
     */
    BlitFn BlitBuffer;

    /**
     * @brief Opaque pointer forwarded to callbacks. Typically points to the
     *        `Server` instance so the callbacks can access the compositing
     *        buffer or IPC handles.
     */
    void* UserData;

    /**
     * @brief When true, window composition must skip blitting the client's
     *        shared content buffer and fall back to solid content-color fill.
     *
     *        Used during drag-resize strip composition: the client reuses
     *        its shared buffer across resizes by reinterpreting it with a
     *        new stride, so reading it from the server mid-drag can race
     *        with the client's redraw and produce scrambled pixels in the
     *        strip. Chrome elements (title bar, buttons, shadow, title
     *        text) still render normally.
     */
    bool SkipContentBlit;
  };
}
