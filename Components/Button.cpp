/**
 * @file Components/Button.cpp
 * @brief Implements the `Button` UI element.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ComponentTypes.hpp"

namespace Quantum::Components {
  const BitmapFont* Button::_font = nullptr;

  Button::Button(
    Canvas& surface,
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height,
    const char* label
  ) :
    Element(surface),
    ClickTarget(
      x,
      y,
      width,
      height
    )
  {
    SetLabel(label);

    _border.AddLayer(OuterBorderColor, 1);
    _border.AddLayer(LightBorderColor, DarkBorderColor, 1);
  }

  void Button::SetPressed(bool pressed) {
    _pressed = pressed;
  }

  void Button::SetPosition(Int16 x, Int16 y) {
    _x = x;
    _y = y;
  }

  void Button::SetLabel(const char* label) {
    Size i = 0;

    if (label) {
      for (; label[i] && i < sizeof(_label) - 1; ++i) {
        _label[i] = label[i];
      }
    }

    _label[i] = '\0';
  }

  void Button::OnPressedChanged(bool pressed) {
    if (_pressed == pressed) return;

    _pressed = pressed;

    Draw();
    Invalidate();
  }

  void Button::Draw() {
    Painter& painter = _canvas.GetPainter();
    UInt32 face = _pressed ? PressedFaceColor : FaceColor;
    UInt16 bt = _border.GetTotalThickness();

    // bezel draws inward from the full button rect
    Rectangle fullRect(_x, _y, _width, _height);
    _border.Draw(_canvas, fullRect);

    // face fills what's left inside the bezel
    Rectangle faceRect(
      static_cast<Int16>(_x + bt),
      static_cast<Int16>(_y + bt),
      static_cast<UInt16>(_width - 2 * bt),
      static_cast<UInt16>(_height - 2 * bt)
    );
    painter.FillRectangle(faceRect, face);

    // measure label
    UInt16 labelLength = 0;

    while (
      _label[labelLength] &&
      labelLength < sizeof(_label) - 1
    ) {
      ++labelLength;
    }

    if (labelLength == 0) return;

    const BitmapFont& font = _font ? *_font : painter.GetFont();
    UInt16 textWidth = font.TextWidth(_label, labelLength);

    Int16 textX = static_cast<Int16>(
      _x + (_width - textWidth) / 2
    );
    Int16 textY = static_cast<Int16>(
      _y + font.CenterTextY(_height)
    );

    if (_font) {
      const BitmapFont& previousFont = painter.GetFont();

      painter.SetFont(*_font);
      painter.DrawText(textX, textY, _label, TextColor, face);
      painter.SetFont(previousFont);
    } else {
      painter.DrawText(textX, textY, _label, TextColor, face);
    }
  }
}
