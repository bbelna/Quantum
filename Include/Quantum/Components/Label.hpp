/**
 * @file Include/Quantum/Components/Label.hpp
 * @brief Declares the `Label` UI element, a single-line plain text string.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Fonts/BitmapFont.hpp>
#include <Quantum/Theme.hpp>
#include <Quantum/UI/Element.hpp>
#include <Quantum/UI/Canvas.hpp>

namespace Quantum::Components {
  /**
   * @brief A single-line text label.
   */
  class Label : public UI::Element {
    public:
      /**
       * @brief Constructs a label at the given position.
       * @param surface The rendering surface.
       * @param x The x-coordinate of the label's left edge.
       * @param y The y-coordinate of the first visible (inked) row of
       *          text. The label compensates for the active font's
       *          internal ascender padding so that `y` is where the
       *          top of the visible text actually lands, not the top
       *          of the glyph cell.
       * @param text The text string to display.
       * @param foregroundColor The foreground text color (ARGB32).
       * @param backgroundColor The background color behind the text (ARGB32).
       */
      Label(
        UI::Canvas& surface,
        Int16 x,
        Int16 y,
        const char* text,
        UInt32 foregroundColor = Theme::TextForeground,
        UInt32 backgroundColor = Theme::TextBackground
      );

      /**
       * @brief Renders the label into its surface.
       */
      void Draw() override;

      /**
       * @brief Returns the label's bounding rectangle.
       */
      Geometry2D::Rectangle GetBounds() const override;

      /**
       * @brief Sets the label's text.
       * @param text The new text string.
       */
      void SetText(const char* text);

      /**
       * @brief Sets the label's position.
       * @param x The new x-coordinate of the label's left edge.
       * @param y The new y-coordinate of the first visible (inked)
       *          row of text. See the constructor for details.
       */
      void SetPosition(Int16 x, Int16 y);

      /**
       * @brief Sets the foreground text color.
       * @param foregroundColor The new ARGB32 color.
       */
      void SetForegroundColor(UInt32 foregroundColor);

      /**
       * @brief Sets a font override for this label. When set, the label
       *        temporarily switches the surface font during @ref Draw and
       *        restores the previous font afterwards.
       * @param font Pointer to the font to use, or `nullptr` to clear the
       *             override and use the surface's current font.
       */
      void SetFont(const Fonts::BitmapFont* font);

    private:
      Int16 _x;

      Int16 _y;

      char _text[128];

      UInt32 _foregroundColor;

      UInt32 _backgroundColor;

      const Fonts::BitmapFont* _font = nullptr;
  };
}
