/**
 * @file Components/TextInput.cpp
 * @brief Implements @ref Quantum::UI::TextInput.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ComponentTypes.hpp"

namespace Quantum::Components {
  TextInput::TextInput(
    Canvas& surface,
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 maxLength,
    UInt32 parentBackground
  ) :
    Element(surface),
    ClickTarget(
      x,
      y,
      width,
      ComputeFieldHeight(surface.GetPainter().GetFont().Height)
    ),
    _maxLength(
      maxLength > BufferSize
        ? BufferSize
        : maxLength
    ),
    _parentBackground(parentBackground)
  {
    _text[0] = '\0';
  }

  UInt16 TextInput::_innerWidth() const {
    return static_cast<UInt16>(_width - TotalHorizontalBorder * 2);
  }

  UInt16 TextInput::_textPixelWidth(UInt16 from, UInt16 to) const {
    const BitmapFont& font = _canvas.GetPainter().GetFont();
    UInt16 width = 0;

    for (
      UInt16 index = from;
      index < to && index < _length;
      ++index
    ) {
      width = static_cast<UInt16>(
        width
        + font.GetAdvance(
          static_cast<UInt8>(_text[index])
        )
      );
    }

    return width;
  }

  void TextInput::_ensureCursorVisible() {
    UInt16 inner = _innerWidth();

    // reserve 1px for the thin bar cursor
    UInt16 available = (inner > 1)
      ? static_cast<UInt16>(inner - 1)
      : 0;

    if (_cursor < _scrollOffset) {
      _scrollOffset = _cursor;
    }

    // scroll right until the cursor fits in the visible area
    while (
      _cursor > _scrollOffset &&
      _textPixelWidth(_scrollOffset, _cursor) > available
    ) {
      _scrollOffset++;
    }

    // scroll back to fill the view when text no longer reaches the right edge
    while (
      _scrollOffset > 0 &&
      _textPixelWidth(
        static_cast<UInt16>(_scrollOffset - 1),
        _length
      ) <= inner
    ) {
      --_scrollOffset;
    }
  }

  void TextInput::_placeCursorAtX(Int16 contentX) {
    Int16 textAreaX = static_cast<Int16>(contentX - TotalHorizontalBorder);

    if (textAreaX < 0) {
      textAreaX = 0;
    }

    const BitmapFont& font = _canvas.GetPainter().GetFont();
    UInt16 accumulated = 0;

    for (
      UInt16 index = _scrollOffset;
      index < _length;
      ++index
    ) {
      UInt8 advance = font.GetAdvance(
        static_cast<UInt8>(_text[index])
      );

      if (accumulated + advance / 2 > static_cast<UInt16>(textAreaX)) {
        _cursor = index;

        return;
      } else {
        accumulated = static_cast<UInt16>(accumulated + advance);
      }
    }

    _cursor = _length;
  }

  void TextInput::SetText(const char* text) {
    _length = 0;

    if (text) {
      for (; text[_length] && _length < _maxLength; ++_length) {
        _text[_length] = text[_length];
      }
    }

    _text[_length] = '\0';
    _cursor = _length;
    _scrollOffset = 0;

    _ensureCursorVisible();
  }

  bool TextInput::HandleMouseDown(Int16 x, Int16 y) {
    if (ClickTarget::HandleMouseDown(x, y)) {
      _focused = true;

      _placeCursorAtX(static_cast<Int16>(x - _x));

      Draw();
      Invalidate();

      return true;
    } else {
      return false;
    }
  }

  void TextInput::SetFocused(bool focused) {
    if (_focused != focused) {
      _focused = focused;

      Draw();
      Invalidate();
    }
  }

  void TextInput::OnPressedChanged(bool) {}

  bool TextInput::ProcessKeyEvent(const Input::InputEvent& event) {
    bool consumed = false;

    if (event.Type == InputEventType::KeyDown) {
      UInt8 scancode = event.Scancode;

      if (scancode == KeyCode::ArrowLeft) {
        if (_cursor > 0) {
          --_cursor;

          _ensureCursorVisible();
        }

        consumed = true;
      } else if (scancode == KeyCode::ArrowRight) {
        if (_cursor < _length) {
          ++_cursor;

          _ensureCursorVisible();
        }

        consumed = true;
      } else if (scancode == KeyCode::Home) {
        _cursor = 0;

        _ensureCursorVisible();

        consumed = true;
      } else if (scancode == KeyCode::End) {
        _cursor = _length;

        _ensureCursorVisible();

        consumed = true;
      } else if (scancode == KeyCode::Backspace) {
        if (_cursor > 0) {
          --_cursor;

          for (
            UInt16 index = _cursor;
            index < _length - 1;
            ++index
          ) {
            _text[index] = _text[index + 1];
          }

          --_length;

          _text[_length] = '\0';

          _ensureCursorVisible();
        }

        consumed = true;
      } else if (scancode == KeyCode::Delete) {
        if (_cursor < _length) {
          for (
            UInt16 index = _cursor;
            index < _length - 1;
            ++index
          ) {
            _text[index] = _text[index + 1];
          }

          --_length;

          _text[_length] = '\0';
        }

        consumed = true;
      } else {
        char character = event.Character;

        if (
          character >= 0x20 &&
          character < 0x7F &&
          _length < _maxLength
        ) {
          for (
            UInt16 index = _length;
            index > _cursor;
            --index
          ) {
            _text[index] = _text[index - 1];
          }

          _text[_cursor] = character;

          ++_length;
          ++_cursor;

          _text[_length] = '\0';

          _ensureCursorVisible();

          consumed = true;
        }
      }
    }

    if (consumed) {
      Draw();
      Invalidate();
    }

    return consumed;
  }

  void TextInput::Draw() {
    Painter& painter = _canvas.GetPainter();
    const BitmapFont& font = painter.GetFont();
    Int16 x = _x;
    Int16 y = _y;
    UInt16 width = _width;
    UInt16 height = _height;

    // fill the full bounding box with the background color; the
    // background fill also overwrites the previous frame's border row
    // when toggling between focused and unfocused colors (the focused
    // 2px border shrinks back to 1px on blur)
    painter.FillRectangle(
      Rectangle(
        x,
        y,
        width,
        height
      ),
      BackgroundColor
    );

    // bottom border only; the focused state mimics the dock active
    // highlight by drawing a 2px stripe that eats up into the bottom
    // padding (the field height already reserves FocusBorderSize so
    // the text doesn't shift on focus changes)
    UInt32 borderColor
      = _focused
      ? FocusBorderColor
      : BorderColor;
    UInt16 borderThickness
      = _focused
      ? FocusBorderSize
      : BorderSize;

    painter.FillRectangle(
      Rectangle(
        x,
        static_cast<Int16>(
          y
          + height
          - borderThickness
        ),
        width,
        borderThickness
      ),
      borderColor
    );

    // draw visible text using DrawText (supports proportional advances)
    Int16 textX = static_cast<Int16>(x + TotalHorizontalBorder);
    Int16 textY = static_cast<Int16>(y + TotalBorder);
    UInt16 inner = _innerWidth();

    // reserve 1px for the thin bar cursor
    UInt16 textArea
      = inner > 1
      ? static_cast<UInt16>(inner - 1)
      : 0;

    // find how many chars from _scrollOffset fit in the visible area
    UInt16 drawCount = 0;
    UInt16 drawWidth = 0;

    for (
      UInt16 index = _scrollOffset;
      index < _length;
      ++index
    ) {
      UInt8 advance = font.GetAdvance(
        static_cast<UInt8>(_text[index])
      );

      if (
        drawWidth + advance > textArea &&
        drawWidth + advance > inner &&
        _cursor <= index
      ) {
        break;
      }

      drawWidth = static_cast<UInt16>(drawWidth + advance);
      drawCount++;
    }

    if (drawCount > 0) {
      // use DrawText which handles proportional advance widths
      // temporarily null-terminate the visible substring
      char saved = _text[_scrollOffset + drawCount];

      _text[_scrollOffset + drawCount] = '\0';

      painter.DrawText(
        textX,
        textY,
        _text + _scrollOffset,
        TextColor,
        BackgroundColor
      );

      _text[_scrollOffset + drawCount] = saved;
    }

    if (_focused) {
      // draw cursor at the correct pixel position
      UInt16 cursorPixelX = _textPixelWidth(
        _scrollOffset,
        _cursor
      );
      Int16 cursorX = static_cast<Int16>(textX + cursorPixelX);

      // thin bar cursor
      painter.FillRectangle(
        Rectangle(
          cursorX,
          textY,
          1,
          font.Height
        ),
        CursorColor
      );
    }
  }
}
