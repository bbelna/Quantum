/**
 * @file Servers/App/Windows/WindowTitleBar.cpp
 * @brief Implements @ref @QAppSrv::Windows::WindowTitleBar.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "WindowTitleBar.hpp"

#include <Quantum/Core/Color.hpp>
#include <Quantum/UI/Canvas.hpp>

namespace Quantum::Servers::App::Windows {
  static constexpr UInt16 ButtonGap = 6;

  WindowTitleBar::WindowTitleBar(
    const WindowTitleBarTheme& theme,
    Rectangle* windowFrame,
    UInt16 bezelTotal,
    const Fonts::BitmapFont* font,
    bool allowClose,
    bool allowMaximize
  ) :
    _theme(theme),
    _windowFrame(windowFrame),
    _bezelTotal(bezelTotal),
    _font(font)
  {
    _bezel.AddLayer(
      0xFF404040,
      0xFF202020,
      1
    );

    _buttons = new WindowTitleBarButtons(windowFrame);

    // the edge inset is the distance from the frame edge to the
    // title bar content area: bezelTotal + title bar padding
    UInt16 edgeRight = static_cast<UInt16>(
      bezelTotal + theme.Padding.Right
    );
    UInt16 edgeTop = static_cast<UInt16>(
      bezelTotal + theme.Padding.Top
    );

    _buttons->SetEdgeInset(edgeRight, edgeTop);

    UInt16 tbHeight = GetHeight();
    UInt16 contentAreaHeight = static_cast<UInt16>(
      tbHeight - theme.Padding.Top - theme.Padding.Bottom
    );

    if (allowClose) {
      _closeButton = new WindowTitleBarButton(
        windowFrame,
        theme.Buttons.CloseColor,
        theme.Buttons.ClosePressedColor,
        WindowTitleBarButtonBitmaps::ButtonWidth,
        WindowTitleBarButtonBitmaps::ButtonHeight
      );

      _closeButton->SetContentAreaHeight(contentAreaHeight);
      _buttons->AddButton(_closeButton);
    }

    if (allowMaximize) {
      _maximizeButton = new WindowTitleBarButton(
        windowFrame,
        theme.Buttons.MaximizeColor,
        theme.Buttons.MaximizePressedColor,
        WindowTitleBarButtonBitmaps::ButtonWidth,
        WindowTitleBarButtonBitmaps::ButtonHeight
      );

      _maximizeButton->SetContentAreaHeight(contentAreaHeight);
      _buttons->AddButton(_maximizeButton);

      _minimizeButton = new WindowTitleBarButton(
        windowFrame,
        theme.Buttons.MinimizeColor,
        theme.Buttons.MinimizePressedColor,
        WindowTitleBarButtonBitmaps::ButtonWidth,
        WindowTitleBarButtonBitmaps::ButtonHeight
      );

      _minimizeButton->SetContentAreaHeight(contentAreaHeight);
      _buttons->AddButton(_minimizeButton);
    }
  }

  WindowTitleBar::~WindowTitleBar() {
    delete _buttons;
  }

  UInt16 WindowTitleBar::GetHeight() const {
    return ComputeTitleBarHeight(_theme, _font);
  }

  void WindowTitleBar::SetActive(bool active) {
    _active = active;

    if (_closeButton) _closeButton->SetActive(active);
    if (_maximizeButton) _maximizeButton->SetActive(active);
    if (_minimizeButton) _minimizeButton->SetActive(active);
  }

  void WindowTitleBar::SetMaximized(bool maximized) {
    _maximized = maximized;
  }

  void WindowTitleBar::SetTitle(const char* title) {
    Size i = 0;

    if (title) {
      for (; title[i] && i < sizeof(_title) - 1; ++i) {
        _title[i] = title[i];
      }
    }

    _title[i] = '\0';
  }

  Rectangle WindowTitleBar::GetFrame() const {
    return Rectangle(
      static_cast<Int16>(_windowFrame->Origin.X + _bezelTotal),
      static_cast<Int16>(_windowFrame->Origin.Y + _bezelTotal),
      static_cast<UInt16>(
        _windowFrame->Dimensions.Width - 2 * _bezelTotal
      ),
      GetHeight()
    );
  }

  // ── IDecoration ─────────────────────────────────────────────────────

  void WindowTitleBar::Draw(
    Canvas& canvas,
    Rectangle contentRectangle
  ) {
    UInt16 height = GetHeight();
    UInt32 fillColor = _active
      ? _theme.ActiveBackground
      : _theme.InactiveBackground;

    // the title bar draws at the top of the rect it receives
    Int16 tbX = contentRectangle.Origin.X;
    Int16 tbY = contentRectangle.Origin.Y;
    UInt16 tbWidth = contentRectangle.Dimensions.Width;
    Rectangle tbRect(tbX, tbY, tbWidth, height);

    Painter& painter = canvas.GetPainter();

    // draw bezel inward from the title bar rect
    _bezel.Draw(canvas, tbRect);

    // fill the interior with the title bar background
    UInt16 bt = _bezel.GetTotalThickness();
    Rectangle tbInterior(
      static_cast<Int16>(tbX + bt),
      static_cast<Int16>(tbY + bt),
      static_cast<UInt16>(tbWidth - 2 * bt),
      static_cast<UInt16>(height - 2 * bt)
    );
    painter.FillRectangle(tbInterior, fillColor);

    // bottom separator
    if (_theme.EnableBottomBorder) {
      painter.FillRectangle(
        Rectangle(
          tbX,
          static_cast<Int16>(tbY + height - 1),
          tbWidth,
          1
        ),
        _theme.SeparatorColor
      );
    }

    // ── traffic-light buttons (right-to-left) ───────────────────────
    UInt16 btnW = WindowTitleBarButtonBitmaps::ButtonWidth;
    UInt16 btnH = WindowTitleBarButtonBitmaps::ButtonHeight;
    UInt16 contentAreaHeight = static_cast<UInt16>(
      height - _theme.Padding.Top - _theme.Padding.Bottom
    );

    Int16 btnY = static_cast<Int16>(
      tbY + _theme.Padding.Top
      + (contentAreaHeight - btnH + 1) / 2
    );

    Int16 cursorRight = static_cast<Int16>(
      tbX + tbWidth - _theme.Padding.Right
    );

    const TrafficLightButtonTheme& btns = _theme.Buttons;

    auto pickColor = [&](
      bool pressed,
      UInt32 normalColor,
      UInt32 pressedColor
    ) -> UInt32 {
      if (!_active) return btns.InactiveColor;
      if (pressed) return pressedColor;
      return normalColor;
    };

    if (_closeButton) {
      cursorRight = static_cast<Int16>(cursorRight - btnW);

      _renderButton(
        canvas, cursorRight, btnY,
        pickColor(
          _closeButton->IsPressed(),
          btns.CloseColor,
          btns.ClosePressedColor
        )
      );

      cursorRight = static_cast<Int16>(cursorRight - ButtonGap);
    }

    if (_maximizeButton) {
      cursorRight = static_cast<Int16>(cursorRight - btnW);

      _renderButton(
        canvas, cursorRight, btnY,
        pickColor(
          _maximizeButton->IsPressed(),
          btns.MaximizeColor,
          btns.MaximizePressedColor
        )
      );

      cursorRight = static_cast<Int16>(cursorRight - ButtonGap);
    }

    if (_minimizeButton) {
      cursorRight = static_cast<Int16>(cursorRight - btnW);

      _renderButton(
        canvas, cursorRight, btnY,
        pickColor(
          _minimizeButton->IsPressed(),
          btns.MinimizeColor,
          btns.MinimizePressedColor
        )
      );

      cursorRight = static_cast<Int16>(cursorRight - ButtonGap);
    }

    // ── title text ──────────────────────────────────────────────────
    Size titleLength = 0;

    while (_title[titleLength] && titleLength < 63) ++titleLength;

    if (titleLength > 0 && _font) {
      UInt32 textColor = _active
        ? _theme.ActiveTextColor
        : _theme.InactiveTextColor;

      UInt16 textWidth = _font->TextWidth(_title, titleLength);

      Int16 buttonLeftEdge = static_cast<Int16>(
        cursorRight - _theme.Padding.Left
      );

      Int16 minLeft = static_cast<Int16>(
        tbX + _theme.Padding.Left
      );

      Int16 textLeft;

      if (_theme.CenterTitle) {
        textLeft = static_cast<Int16>(
          tbX + (tbWidth - static_cast<Int16>(textWidth)) / 2
        );
      } else {
        textLeft = minLeft;
      }

      Int16 maxLeft = static_cast<Int16>(
        buttonLeftEdge - static_cast<Int16>(textWidth)
      );

      if (textLeft > maxLeft) textLeft = maxLeft;
      if (textLeft < minLeft) textLeft = minLeft;

      Int16 textY = static_cast<Int16>(
        tbY + _theme.Padding.Top
        + _font->CenterTextY(contentAreaHeight)
      );

      painter.SetFont(*_font);
      painter.DrawText(
        textLeft, textY, _title,
        textColor, fillColor
      );
    }
  }

  Rectangle WindowTitleBar::ExpandBounds(
    Rectangle contentRectangle
  ) const {
    UInt16 height = GetHeight();

    return Rectangle(
      contentRectangle.Origin.X,
      static_cast<Int16>(contentRectangle.Origin.Y - height),
      contentRectangle.Dimensions.Width,
      static_cast<UInt16>(contentRectangle.Dimensions.Height + height)
    );
  }

  void WindowTitleBar::_renderButton(
    Canvas& canvas,
    Int16 x,
    Int16 y,
    UInt32 color
  ) const {
    UInt16 btnW = WindowTitleBarButtonBitmaps::ButtonWidth;
    UInt16 btnH = WindowTitleBarButtonBitmaps::ButtonHeight;

    UInt32 stampBuffer[WindowTitleBarButtonBitmaps::PixelCount];
    WindowTitleBarButtonBitmaps::Stamp(stampBuffer, color);

    UInt32* canvasPixels = static_cast<UInt32*>(canvas.GetPixels());
    UInt16 canvasStride = canvas.GetStride();
    UInt16 canvasWidth = canvas.GetWidth();
    UInt16 canvasHeight = canvas.GetHeight();

    for (UInt16 row = 0; row < btnH; ++row) {
      Int16 py = static_cast<Int16>(y + row);

      if (py < 0 || py >= static_cast<Int16>(canvasHeight)) continue;

      for (UInt16 col = 0; col < btnW; ++col) {
        Int16 px = static_cast<Int16>(x + col);

        if (px < 0 || px >= static_cast<Int16>(canvasWidth)) continue;

        UInt32 srcPixel = stampBuffer[row * btnW + col];
        UInt8 alpha = static_cast<UInt8>((srcPixel >> 24) & 0xFF);

        if (alpha == 0) continue;

        UInt32 dstIndex
          = static_cast<UInt32>(py) * canvasStride + px;

        if (alpha == 0xFF) {
          canvasPixels[dstIndex] = srcPixel;
        } else {
          canvasPixels[dstIndex] = Color::BlendOver(
            srcPixel, canvasPixels[dstIndex], alpha
          );
        }
      }
    }
  }
}
