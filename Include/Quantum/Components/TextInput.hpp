/**
 * @file Include/Quantum/Components/TextInput.hpp
 * @brief Declares @ref Quantum::UI::TextInput.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Input.hpp>
#include <Quantum/Core/Types.hpp>
#include <Quantum/Theme.hpp>
#include <Quantum/UI/ClickTarget.hpp>
#include <Quantum/UI/Element.hpp>
#include <Quantum/UI/Canvas.hpp>

namespace Quantum::Components {
  /**
   * @brief A single-line text input field.
   */
  class TextInput :
    public UI::Element,
    public UI::ClickTarget
  {
    public:
      /**
       * @brief The background color of the text input (ARGB32 format).
       */
      static constexpr UInt32 BackgroundColor = 0xFF404040;

      /**
       * @brief The text color of the text input (ARGB32 format).
       */
      static constexpr UInt32 TextColor = 0xFFEEEEEE;

      /**
       * @brief The color of the text cursor (ARGB32 format).
       */
      static constexpr UInt32 CursorColor = 0xFFEEEEEE;

      /**
       * @brief The color of the text cursor when focused (ARGB32 format).
       */
      static constexpr UInt32 CursorTextColor = 0xFF000000;

      /**
       * @brief The border color (normal state).
       */
      static constexpr UInt32 BorderColor = Theme::TextInputBorderColor;

      /**
       * @brief The border color when focused.
       */
      static constexpr UInt32 FocusBorderColor
        = Theme::TextInputFocusBorderColor;

      /**
       * @brief The corner radius in pixels.
       */
      static constexpr UInt8 CornerRadius = Theme::TextInputCornerRadius;

      /**
       * @brief The thickness of the bottom border in the unfocused
       *        state, in pixels.
       */
      static constexpr UInt16 BorderSize = 1;

      /**
       * @brief The thickness of the bottom border in the focused
       *        state, in pixels. The focused border eats into the
       *        bottom padding so the field height stays constant.
       */
      static constexpr UInt16 FocusBorderSize = 2;

      /**
       * @brief The vertical padding above the text in pixels
       *        (independent of border thickness).
       */
      static constexpr UInt16 TopPadding = 4;

      /**
       * @brief The vertical padding below the text in pixels
       *        (independent of border thickness). Sized one pixel
       *        larger than the top to balance the visual weight of
       *        the bottom border.
       */
      static constexpr UInt16 BottomPadding = 4;

      /**
       * @brief The horizontal padding between the border and text in pixels.
       */
      static constexpr UInt16 HorizontalPadding = 4;

      /**
       * @brief The maximum number of characters allowed in the text input.
       */
      static constexpr UInt16 BufferSize = 255;

      /**
       * @brief The Y offset from the top of the text input to the
       *        first text row. There is no top border so this is just
       *        the top padding.
       */
      static constexpr UInt16 TotalBorder = TextInput::TopPadding;

      /**
       * @brief The total horizontal non-text width per side
       *        (border + horizontal padding).
       */
      static constexpr UInt16 TotalHorizontalBorder
        = TextInput::BorderSize + TextInput::HorizontalPadding;

      /**
       * @brief Returns the total height of the text input in pixels.
       *        Sized for the unfocused 1px border with full top and
       *        bottom padding; the focused 2px border eats one row of
       *        bottom padding rather than growing the field.
       * @param glyphHeight The active font's glyph height.
       */
      static constexpr UInt16 ComputeFieldHeight(UInt8 glyphHeight) {
        return static_cast<UInt16>(
          TopPadding + glyphHeight + BottomPadding + BorderSize
        );
      }

      /**
       * @brief Creates a new @ref TextInput instance.
       * @param surface The rendering surface.
       * @param x The x-coordinate of the top-left corner.
       * @param y The y-coordinate of the top-left corner.
       * @param width The total width in pixels (including border).
       * @param maxLength Maximum number of characters allowed (1-255).
       * @param parentBackground The background color of the parent
       *        surface, used to clear stale AA fringe on redraw.
       */
      TextInput(
        UI::Canvas& surface,
        Int16 x,
        Int16 y,
        UInt16 width,
        UInt16 maxLength = BufferSize,
        UInt32 parentBackground = Theme::WindowBackground
      );

      /**
       * @brief Renders the text input into its surface.
       */
      void Draw() override;

      /**
       * @brief Returns this text input as a click target for mouse event
       *        routing.
       */
      ClickTarget* AsClickTarget() override { return this; }

      /**
       * @brief Returns whether this text input accepts keyboard input.
       */
      bool AcceptsKeyboard() const override { return true; }

      /**
       * @brief Returns the text input's bounding rectangle.
       */
      Geometry2D::Rectangle GetBounds() const override {
        return Geometry2D::Rectangle(_x, _y, _width, _height);
      }

      /**
       * @brief Processes a keyboard event.
       * @param event The input event to process.
       * @return `true` if the event was consumed; `false` otherwise.
       */
      bool ProcessKeyEvent(const Input::InputEvent& event) override;

      /**
       * @brief Gets a pointer to the current text content.
       * @return Null-terminated string of the current text (up to `BufferSize`
       *         characters). The caller should not modify this string.
       */
      const char* GetText() const { return _text; }

      /**
       * @brief Gets the current text length.
       * @return The number of characters currently in the text input.
       */
      UInt16 GetLength() const { return _length; }

      /**
       * @brief Sets the text content, replacing any existing text.
       * @param text The new text string.
       */
      void SetText(const char* text);

      /**
       * @brief Handles a mouse-down event.
       * @param x The content-relative x-coordinate.
       * @param y The content-relative y-coordinate.
       * @return `true` if the click was inside the text input; `false`
       *         otherwise.
       * 
       * Positions the cursor at the clicked character if the click is inside
       * the text input.
       */
      bool HandleMouseDown(Int16 x, Int16 y) override;

      /**
       * @brief Sets whether this text input has keyboard focus.
       * @param focused `true` to show the cursor, `false` to hide it.
       */
      void SetFocused(bool focused);

      /**
       * @brief Called by the owning window when focus changes. Delegates
       *        to @ref SetFocused.
       * @param focused `true` if gaining focus; `false` if losing it.
       */
      void OnFocusChanged(bool focused) override { SetFocused(focused); }

      /**
       * @brief Indicates if this text input currently has focus.
       * @return `true` if the cursor is visible; `false` otherwise.
       */
      bool IsFocused() const { return _focused; }

    protected:
      void OnPressedChanged(bool pressed) override;

    private:
      char _text[BufferSize + 1];
      UInt16 _maxLength = BufferSize;
      UInt16 _length = 0;
      UInt16 _cursor = 0;
      UInt16 _scrollOffset = 0;
      bool _focused = false;
      UInt32 _parentBackground;

      UInt16 _innerWidth() const;
      UInt16 _textPixelWidth(UInt16 from, UInt16 to) const;
      void _ensureCursorVisible();
      void _placeCursorAtX(Int16 relativeX);
  };
}
