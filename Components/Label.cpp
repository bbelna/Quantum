/**
 * @file Components/Label.cpp
 * @brief Implements @ref @QComponents::Label.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ComponentTypes.hpp"

namespace Quantum::Components {
  Label::Label(
    Canvas& surface,
    Int16 x,
    Int16 y,
    const char* text,
    UInt32 foregroundColor,
    UInt32 backgroundColor
  ) :
    Element(surface),
    _x(x),
    _y(y),
    _foregroundColor(foregroundColor),
    _backgroundColor(backgroundColor)
  {
    SetText(text);
  }

  void Label::SetText(const char* text) {
    Size i = 0;

    if (text) {
      for (; text[i] && i < sizeof(_text) - 1; ++i) {
        _text[i] = text[i];
      }
    }

    _text[i] = '\0';
  }

  void Label::SetPosition(Int16 x, Int16 y) {
    _x = x;
    _y = y;
  }

  void Label::SetForegroundColor(UInt32 color) {
    _foregroundColor = color;
  }

  void Label::SetFont(const BitmapFont* font) {
    _font = font;
  }

  Rectangle Label::GetBounds() const {
    UInt16 length = 0;

    while (_text[length] && length < sizeof(_text) - 1) ++length;

    // _y is the visible (ink) top of the text, so the drawn glyph cell
    // starts InkTop rows above _y. report the cell region so invalidation
    // covers every pixel that Draw actually paints, including descenders
    const BitmapFont* activeFont = _font ? _font : &_canvas.GetPainter().GetFont();

    if (activeFont && activeFont->Height > 0) {
      UInt16 width = activeFont->TextWidth(_text);

      return Rectangle(
        _x,
        static_cast<Int16>(_y - activeFont->InkTop),
        width,
        activeFont->Height
      );
    }

    return Rectangle(
      _x,
      _y,
      static_cast<UInt16>(length * 8),
      14
    );
  }

  void Label::Draw() {
    if (_text[0] == '\0') return;

    // translate _y (visible ink top) to the glyph cell top that
    // DrawText expects by subtracting the font's InkTop. this keeps
    // the Label's Y semantics intuitive — e.g. y = 12 puts the first
    // inked row 12 pixels below the surface top — rather than leaking
    // the font's internal ascender padding into the caller's layout
    Painter& painter = _canvas.GetPainter();

    if (_font) {
      const BitmapFont& previous = painter.GetFont();

      painter.SetFont(*_font);
      painter.DrawText(
        _x,
        static_cast<Int16>(_y - _font->InkTop),
        _text,
        _foregroundColor,
        _backgroundColor
      );
      painter.SetFont(previous);
    } else {
      painter.DrawText(
        _x,
        static_cast<Int16>(_y - painter.GetFont().InkTop),
        _text,
        _foregroundColor,
        _backgroundColor
      );
    }
  }
}
