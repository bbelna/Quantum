/**
 * @file Include/Quantum/UI/Painter.hpp
 * @brief Declares @ref @QUI::Painter.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Fonts.hpp>
#include <Quantum/Geometry2D.hpp>
#include "UIConstants.hpp"

namespace Quantum::UI {
  class Canvas;

  /**
   * @brief The rendering tool for a @ref Canvas surface.
   *
   * A Painter is obtained from a Canvas via @ref Canvas::GetPainter and
   * performs all drawing operations onto that surface.  It holds its own
   * rendering state (currently a font) and reads the underlying pixel
   * buffer through the owning Canvas's accessors.
   *
   * @note Painters are created and owned by Canvas — do not construct
   *       them directly.
   */
  class Painter {
    public:
      /**
       * @brief Returns the global default font, or the built-in default
       *        if none has been set via @ref SetDefaultFont.
       */
      static const Fonts::BitmapFont* GetDefaultFont();

      /**
       * @brief Sets the global default font used by all newly created
       *        painters. Call once after loading the system font.
       * @param font The font to use as the default. Must remain valid
       *             for the lifetime of the application.
       */
      static void SetDefaultFont(const Fonts::BitmapFont* font);

      /**
       * @brief Fills a rectangle with an ARGB32 color, clamped to the
       *        active area.
       * @param rectangle The rectangle to fill.
       * @param color The ARGB32 color.
       */
      void FillRectangle(Geometry2D::Rectangle rectangle, UInt32 color);

      /**
       * @brief Fills a rectangle with rounded corners, clamped to the active
       *        area.
       * @note Pixels outside the rounded corners are left untouched.
       * @param rectangle The rectangle to fill.
       * @param color The ARGB32 color.
       * @param radius The corner radius in pixels. Clamped to half the
       *               smallest dimension.
       * @param corners Which corners to round. Non-flagged corners are
       *                drawn square.
       */
      void FillRoundedRectangle(
        Geometry2D::Rectangle rectangle,
        UInt32 color,
        UInt8 radius,
        RoundedCorners corners = RoundedCorners::All
      );

      /**
       * @brief Renders a soft drop shadow into the surface around a
       *        content rectangle.
       * @param contentRectangle The content rectangle within the surface
       *        (where the shadow should not appear).
       * @param shadowSize Number of shadow layers (pixels of spread).
       * @param peakAlpha Peak alpha of the innermost shadow layer.
       * @param cornerRadius Corner radius of the content shape.
       * @param offsetX Horizontal offset (positive = right).
       * @param offsetY Vertical offset (positive = down).
       * @param corners Which corners of the content to round.
       */
      void RenderShadow(
        Geometry2D::Rectangle contentRectangle,
        UInt8 shadowSize,
        UInt8 peakAlpha,
        UInt8 cornerRadius,
        Int16 offsetX = 0,
        Int16 offsetY = 0,
        RoundedCorners corners = RoundedCorners::All
      );

      /**
       * @brief Returns the active font used for text rendering.
       */
      const Fonts::BitmapFont& GetFont() const { return _font; }

      /**
       * @brief Replaces the active font used for text rendering.
       * @param font The font to copy.
       */
      void SetFont(const Fonts::BitmapFont& font);

      /**
       * @brief Renders a single glyph using the active font.
       * @param x Pixel x-coordinate of the glyph's top-left corner.
       * @param y Pixel y-coordinate of the glyph's top-left corner.
       * @param character Character code (indexes the active font).
       * @param foregroundColor ARGB32 foreground color.
       * @param backgroundColor ARGB32 background color.
       */
      void DrawGlyph(
        UInt16 x,
        UInt16 y,
        char character,
        UInt32 foregroundColor,
        UInt32 backgroundColor,
        bool bold = false
      );

      /**
       * @brief Renders @p text using the active font.
       * @param x Pixel x-coordinate of the first glyph's top-left corner.
       * @param y Pixel y-coordinate of the first glyph's top-left corner.
       * @param text The null-terminated string to render.
       * @param foregroundColor Foreground color as an ARGB32 value.
       * @param backgroundColor Background color as an ARGB32 value.
       */
      void DrawText(
        Int16 x,
        Int16 y,
        const char* text,
        UInt32 foregroundColor,
        UInt32 backgroundColor,
        bool bold = false
      );

      /**
       * @brief Returns the pixel width of a null-terminated string using
       *        the active font.
       * @param text The string to measure.
       * @return Total width in pixels.
       */
      UInt16 TextWidth(const char* text) const {
        return _font.TextWidth(text);
      }

      /**
       * @brief Returns the pixel width of a counted string using the
       *        active font.
       * @param text The string to measure.
       * @param count Maximum number of characters to measure.
       * @return Total width in pixels.
       */
      UInt16 TextWidth(const char* text, Size count) const {
        return _font.TextWidth(text, count);
      }

      /**
       * @brief Fills the entire active area with a solid color.
       * @param color The 32-bit ARGB color value.
       */
      void Clear(UInt32 color);

      /**
       * @brief Scrolls the canvas contents up by the given number of pixel
       *        rows. The exposed bottom strip is filled with the given color.
       * @param rows Number of pixel rows to scroll up.
       * @param fillColor Color to fill the exposed bottom strip.
       */
      void ScrollUp(UInt16 rows, UInt32 fillColor);

      /**
       * @brief Scrolls the canvas contents down by the given number of pixel
       *        rows. The exposed top strip is filled with the given color.
       * @param rows Number of pixel rows to scroll down.
       * @param fillColor Color to fill the exposed top strip.
       */
      void ScrollDown(UInt16 rows, UInt32 fillColor);

    private:
      friend class Canvas;

      /**
       * @brief Constructs a painter bound to the given canvas.
       * @param canvas The canvas to draw onto.
       */
      explicit Painter(Canvas& canvas);

      /**
       * @brief Back-pointer to the owning canvas.
       */
      Canvas* _canvas;

      /**
       * @brief Active font used for text rendering (owned copy).
       */
      Fonts::BitmapFont _font;
  };
}
