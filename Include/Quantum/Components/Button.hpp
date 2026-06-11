/**
 * @file Include/Quantum/Components/Button.hpp
 * @brief Declares @ref @QComponents::Button.
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
#include <Quantum/UI/Bezel.hpp>
#include <Quantum/UI/ClickTarget.hpp>
#include <Quantum/UI/Element.hpp>
#include <Quantum/UI/Canvas.hpp>

namespace Quantum::Components {
  /**
   * @brief Button component.
   */
  class Button :
    public UI::Element,
    public UI::ClickTarget
  {
    public:
      /**
       * @brief Sets the font used for all button labels. If not set,
       *        buttons fall back to the surface's current font.
       * @param font The font to use. Must remain valid.
       */
      static void SetFont(const Fonts::BitmapFont* font) {
        _font = font;
      }

      static constexpr UInt32 FaceColor = 0xFF404040;
      static constexpr UInt32 LightBorderColor = 0xFF707070;
      static constexpr UInt32 DarkBorderColor = 0xFF202020;
      static constexpr UInt32 OuterBorderColor = 0xFF000000;
      static constexpr UInt32 PressedFaceColor = 0xFF101010;
      static constexpr UInt32 TextColor = Theme::ButtonGlyph;
      static constexpr UInt8 CornerRadius = Theme::ButtonCornerRadius;

      /**
       * @brief Constructs a button at the given position and size.
       * @param surface The rendering surface.
       * @param x The x-coordinate of the button's top-left corner.
       * @param y The y-coordinate of the button's top-left corner.
       * @param width The width of the button in pixels.
       * @param height The height of the button in pixels.
       * @param label The text label to display on the button.
       */
      Button(
        UI::Canvas& surface,
        Int16 x,
        Int16 y,
        UInt16 width,
        UInt16 height,
        const char* label
      );

      /**
       * @brief Renders the button into its surface.
       */
      void Draw() override;

      /**
       * @brief Returns this button as a click target for mouse event
       *        routing.
       */
      ClickTarget* AsClickTarget() override { return this; }

      /**
       * @brief Returns the button's bounding rectangle.
       */
      Geometry2D::Rectangle GetBounds() const override {
        return Geometry2D::Rectangle(
          _x,
          _y,
          _width,
          _height
        );
      }

      /**
       * @brief Sets the pressed (sunken) state of the button.
       * @param pressed `true` for pressed appearance, `false` for normal.
       */
      void SetPressed(bool pressed);

      /**
       * @brief Returns whether the button is currently in the pressed state.
       */
      bool IsPressed() const { return _pressed; }

      /**
       * @brief Sets the button's position.
       * @param x The new x-coordinate.
       * @param y The new y-coordinate.
       */
      void SetPosition(Int16 x, Int16 y);

      /**
       * @brief Sets the button's label text.
       * @param label The new label string.
       */
      void SetLabel(const char* label);

    protected:
      /**
       * @brief Called by `ClickTarget` when the pressed visual state changes.
       * @param pressed `true` if entering the pressed state.
       */
      void OnPressedChanged(bool pressed) override;

    private:
      static const Fonts::BitmapFont* _font;

      char _label[32];
      bool _pressed = false;
      UI::Bezel _border;
  };
}
