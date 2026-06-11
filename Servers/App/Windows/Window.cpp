/**
 * @file Servers/App/Windows/Window.cpp
 * @brief Implements @ref @QAppSrv::Window.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Window.hpp"

#include <Quantum/UI/Bezel.hpp>
#include <Quantum/UI/Canvas.hpp>

namespace Quantum::Servers::App::Windows {
  static UInt16 BezelThicknessFromTheme(const BorderTheme& border) {
    UInt16 total = border.Thickness;

    if (border.EnableOuterBorder) total = static_cast<UInt16>(total + 1);
    if (border.InnerColor != 0) {
      total = static_cast<UInt16>(total + border.InnerThickness);
    }

    return total;
  }

  static const Fonts::BitmapFont* _titleFont = nullptr;

  void Window::SetTitleFont(const Fonts::BitmapFont* font) {
    _titleFont = font;
  }

  const Fonts::BitmapFont& Window::GetTitleFont() {
    return _titleFont ? *_titleFont : Fonts::DefaultFont;
  }

  UInt16 Window::GetTitleBarHeight(const WindowTheme& theme) {
    return ComputeTitleBarHeight(theme.TitleBar, &GetTitleFont());
  }

  UInt16 Window::EffectiveOuterInset(
    bool maximized,
    const WindowTheme& theme
  ) {
    if (maximized) return 0;

    return 0;
  }

  UInt16 Window::ContentWidthFromChromeWidth(
    UInt16 chromeWidth,
    bool maximized,
    const WindowTheme& theme
  ) {
    UInt16 bezel = maximized
      ? 0
      : BezelThicknessFromTheme(theme.Border);

    return static_cast<UInt16>(chromeWidth - 2 * bezel);
  }

  UInt16 Window::ContentHeightFromChromeHeight(
    UInt16 chromeHeight,
    bool maximized,
    const WindowTheme& theme
  ) {
    UInt16 bezel = maximized
      ? 0
      : BezelThicknessFromTheme(theme.Border);
    UInt16 tbHeight = GetTitleBarHeight(theme);

    return static_cast<UInt16>(
      chromeHeight - 2 * bezel - tbHeight
    );
  }

  Window::Window(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height,
    const char* title,
    const WindowTheme& theme,
    bool allowClose,
    bool allowMaximize,
    bool allowResize,
    bool chromeless
  ) :
    _theme(theme),
    _frame(x, y, width, height),
    _allowResize(allowResize),
    _chromeless(chromeless),
    _contentColor(theme.Background)
  {
    Size i = 0;

    if (title) {
      for (; title[i] && i < sizeof(_title) - 1; ++i) {
        _title[i] = title[i];
      }
    }

    _title[i] = '\0';

    if (_chromeless) {
      _contentReady = true;
      return;
    }

    UInt16 bezelTotal = BezelThicknessFromTheme(theme.Border);

    _titleBar = new WindowTitleBar(
      theme.TitleBar,
      &_frame,
      bezelTotal,
      &GetTitleFont(),
      allowClose,
      allowMaximize
    );
    _titleBar->SetTitle(_title);

    if (!allowMaximize) _allowResize = false;

    _resizeGrip = new WindowResizeGrip(
      theme.ResizeGrip,
      &_frame
    );
    _resizeGrip->SetEnabled(_allowResize);

    _borderBezel = new UI::Bezel();

    _borderBezel->AddLayer(
      theme.Border.OuterBorderColor,
      1
    );
    _borderBezel->AddLayer(
      theme.Bezel.LightColor,
      theme.Bezel.DarkColor,
      1
    );

    _decorations[_decorationCount++] = _borderBezel;
    _decorations[_decorationCount++] = _titleBar;
    _decorations[_decorationCount++] = _resizeGrip;

    _rebuildChrome();
  }

  Window::~Window() {
    delete[] _chromeBuffer;
    delete _borderBezel;
    delete _titleBar;
    delete _resizeGrip;
  }

  void Window::SetActive(bool active) {
    _active = active;

    if (_titleBar) _titleBar->SetActive(active);
    if (!_chromeless) _rebuildChrome();
  }

  void Window::SetPosition(Int16 x, Int16 y) {
    _frame.Origin = Point(x, y);
  }

  void Window::SetSize(UInt16 width, UInt16 height) {
    _frame.Dimensions = Dimensions(width, height);

    if (!_chromeless) _rebuildChrome();
  }

  void Window::SetMaximized(
    bool maximized,
    UInt16 screenWidth,
    UInt16 screenHeight,
    UInt16 topOffset,
    UInt16 bottomOffset
  ) {
    if (maximized && !_maximized) {
      _restoreFrame = _frame;
      _maximized = true;

      if (_titleBar) {
        _titleBar->SetMaximized(true);
        _titleBar->GetButtons()->SetEdgeInset(
          static_cast<UInt16>(_theme.TitleBar.Padding.Right),
          static_cast<UInt16>(_theme.TitleBar.Padding.Top)
        );
      }

      if (_resizeGrip) _resizeGrip->SetEnabled(false);

      SetPosition(0, static_cast<Int16>(topOffset));
      SetSize(
        screenWidth,
        static_cast<UInt16>(screenHeight - topOffset - bottomOffset)
      );
    } else if (!maximized && _maximized) {
      _maximized = false;

      if (_titleBar) {
        _titleBar->SetMaximized(false);

        UInt16 dt = GetDecorationThickness();

        _titleBar->GetButtons()->SetEdgeInset(
          static_cast<UInt16>(dt + _theme.TitleBar.Padding.Right),
          static_cast<UInt16>(dt + _theme.TitleBar.Padding.Top)
        );
      }

      if (_resizeGrip) _resizeGrip->SetEnabled(_allowResize);

      SetPosition(
        _restoreFrame.Origin.X,
        _restoreFrame.Origin.Y
      );
      SetSize(
        _restoreFrame.Dimensions.Width,
        _restoreFrame.Dimensions.Height
      );
    }
  }

  UInt16 Window::_getOuterInset() const {
    return EffectiveOuterInset(_maximized, _theme);
  }

  UInt16 Window::_getTitleBarHeight() const {
    return GetTitleBarHeight(_theme);
  }

  UInt16 Window::GetDecorationThickness() const {
    if (_chromeless || _decorationsDrawn == 0) return 0;

    Rectangle inner(0, 0, 0, 0);

    for (Size i = 0; i < _decorationsDrawn; ++i) {
      inner = _decorations[i]->ExpandBounds(inner);
    }

    return static_cast<UInt16>(-inner.Origin.X);
  }


  Rectangle Window::GetChromeFrame() const {
    if (_chromeless) return _frame;

    return _frame;
  }

  UInt16 Window::GetButtonMarginWidth() const {
    if (_chromeless || !_titleBar) return 0;

    return static_cast<UInt16>(
      _frame.GetRight()
      - _titleBar->GetButtons()->GetLeftEdge()
    );
  }

  Rectangle Window::_getContentFrame() const {
    if (_chromeless) return _frame;

    // walk the decoration chain, shrinking from the frame inward
    Rectangle remaining = _frame;

    for (Size i = 0; i < _decorationCount; ++i) {
      if (_maximized && _decorations[i] == _borderBezel) continue;

      Rectangle zero(0, 0, 0, 0);
      Rectangle expanded = _decorations[i]->ExpandBounds(zero);

      Int16 left = static_cast<Int16>(-expanded.Origin.X);
      Int16 top = static_cast<Int16>(-expanded.Origin.Y);
      UInt16 totalW = expanded.Dimensions.Width;
      UInt16 totalH = expanded.Dimensions.Height;

      remaining = Rectangle(
        static_cast<Int16>(remaining.Origin.X + left),
        static_cast<Int16>(remaining.Origin.Y + top),
        static_cast<UInt16>(remaining.Dimensions.Width - totalW),
        static_cast<UInt16>(remaining.Dimensions.Height - totalH)
      );
    }

    return remaining;
  }

  bool Window::HitTest(Int16 px, Int16 py) const {
    if (_minimized) return false;

    return _frame.Contains(Point(px, py));
  }

  bool Window::HitTestTitleBar(Int16 px, Int16 py) const {
    if (_chromeless || !_titleBar) return false;

    Point p(px, py);

    return _titleBar->GetFrame().Contains(p)
        && !_titleBar->GetButtons()->Contains(p);
  }

  bool Window::HitTestCloseButton(Int16 px, Int16 py) const {
    if (_chromeless || !_titleBar) return false;

    WindowTitleBarButton* btn = _titleBar->GetCloseButton();

    return btn && btn->GetFrame().Contains(Point(px, py));
  }

  bool Window::HitTestMaximizeButton(Int16 px, Int16 py) const {
    if (_chromeless || !_titleBar) return false;

    WindowTitleBarButton* btn = _titleBar->GetMaximizeButton();

    return btn && btn->GetFrame().Contains(Point(px, py));
  }

  bool Window::HitTestMinimizeButton(Int16 px, Int16 py) const {
    if (_chromeless || !_titleBar) return false;

    WindowTitleBarButton* btn = _titleBar->GetMinimizeButton();

    return btn && btn->GetFrame().Contains(Point(px, py));
  }

  bool Window::HitTestResizeHandle(Int16 px, Int16 py) const {
    if (_chromeless || !_allowResize || _maximized) return false;

    return _resizeGrip->GetFrame().Contains(Point(px, py));
  }

  WindowTitleBarButton* Window::GetCloseButton() const {
    return _titleBar ? _titleBar->GetCloseButton() : nullptr;
  }

  WindowTitleBarButton* Window::GetMaximizeButton() const {
    return _titleBar ? _titleBar->GetMaximizeButton() : nullptr;
  }

  WindowTitleBarButton* Window::GetMinimizeButton() const {
    return _titleBar ? _titleBar->GetMinimizeButton() : nullptr;
  }

  void Window::SetContentBuffer(
    void* buffer,
    UInt16 width,
    UInt16 height,
    UInt16 stride,
    UInt8 bytesPerPixel
  ) {
    _contentBuffer = buffer;
    _contentWidth = width;
    _contentHeight = height;
    _contentStride = stride;
    _contentBPP = bytesPerPixel;
  }

  void Window::DrawClipped(
    Rectangle clipRectangle,
    const DrawContext& context
  ) const {
    Rectangle contentFrame = _getContentFrame();

    bool contentCoversClip
      = _contentBuffer
     && _contentWidth > 0
     && _contentHeight > 0
     && _contentWidth >= contentFrame.Dimensions.Width
     && _contentHeight >= contentFrame.Dimensions.Height
     && context.BlitBuffer
     && !context.SkipContentBlit
     && contentFrame.Contains(clipRectangle);

    if (!contentCoversClip) {
      Rectangle visible = _frame.Intersect(clipRectangle);

      if (!visible.IsEmpty()) {
        context.FillRectangle(
          context.UserData, visible, _contentColor
        );
      }

      if (_innerContentColor != 0) {
        Rectangle contentInner
          = contentFrame.Intersect(clipRectangle);

        if (!contentInner.IsEmpty()) {
          context.FillRectangle(
            context.UserData, contentInner, _innerContentColor
          );
        }
      }
    }

    if (
      _contentBuffer &&
      _contentWidth > 0 &&
      _contentHeight > 0 &&
      context.BlitBuffer &&
      !context.SkipContentBlit
    ) {
      Rectangle content = contentFrame.Intersect(clipRectangle);

      if (!content.IsEmpty()) {
        Int16 sourceX = static_cast<Int16>(
          content.Origin.X - contentFrame.Origin.X
        );
        Int16 sourceY = static_cast<Int16>(
          content.Origin.Y - contentFrame.Origin.Y
        );

        if (
          sourceX >= 0 &&
          sourceY >= 0 &&
          static_cast<UInt16>(sourceX) < _contentWidth &&
          static_cast<UInt16>(sourceY) < _contentHeight
        ) {
          UInt16 blitWidth = content.Dimensions.Width;
          UInt16 blitHeight = content.Dimensions.Height;
          UInt16 availableWidth = static_cast<UInt16>(
            _contentWidth - static_cast<UInt16>(sourceX)
          );
          UInt16 availableHeight = static_cast<UInt16>(
            _contentHeight - static_cast<UInt16>(sourceY)
          );

          if (blitWidth > availableWidth) blitWidth = availableWidth;
          if (blitHeight > availableHeight) blitHeight = availableHeight;

          UInt32 pixelOffset
            = static_cast<UInt32>(sourceY) * _contentStride
            + static_cast<UInt16>(sourceX);
          const void* source
            = static_cast<const UInt8*>(_contentBuffer)
            + pixelOffset * _contentBPP;

          context.BlitBuffer(
            context.UserData,
            content.Origin.X,
            content.Origin.Y,
            source,
            blitWidth,
            blitHeight,
            _contentStride,
            _contentBPP
          );
        }
      }
    }

    if (_chromeless || !_chromeBuffer || !context.BlitBuffer) return;

    Rectangle chromeRect(
      _frame.Origin.X,
      _frame.Origin.Y,
      _chromeWidth,
      _chromeHeight
    );
    Rectangle visible = chromeRect.Intersect(clipRectangle);

    if (visible.IsEmpty()) return;

    Int16 srcX = static_cast<Int16>(
      visible.Origin.X - _frame.Origin.X
    );
    Int16 srcY = static_cast<Int16>(
      visible.Origin.Y - _frame.Origin.Y
    );

    const UInt8* src
      = reinterpret_cast<const UInt8*>(_chromeBuffer)
      + (static_cast<UInt32>(srcY) * _chromeWidth + srcX) * 4;

    context.BlitBuffer(
      context.UserData,
      visible.Origin.X,
      visible.Origin.Y,
      src,
      visible.Dimensions.Width,
      visible.Dimensions.Height,
      _chromeWidth,
      4
    );
  }

  void Window::_rebuildChrome() {
    if (_chromeless) return;

    _chromeWidth = _frame.Dimensions.Width;
    _chromeHeight = _frame.Dimensions.Height;

    Size pixelCount
      = static_cast<Size>(_chromeWidth) * _chromeHeight;

    if (pixelCount > _chromeBufferCapacity) {
      delete[] _chromeBuffer;

      constexpr Size Granularity = 16384;
      Size roundedCount = AlignUp(pixelCount, Granularity);

      _chromeBuffer = new UInt32[roundedCount];

      if (!_chromeBuffer) {
        _chromeWidth = 0;
        _chromeHeight = 0;
        _chromeBufferCapacity = 0;
        return;
      }

      _chromeBufferCapacity = roundedCount;
    }

    // fill with the window background color so the padding area
    // between the frame edge and the bezel is opaque and matches
    // the content color; decorations draw on top of this base
    for (Size i = 0; i < pixelCount; ++i) {
      _chromeBuffer[i] = _contentColor;
    }

    UI::Canvas chromeCanvas(
      _chromeBuffer, _chromeWidth, _chromeHeight, _chromeWidth
    );

    // start with the full canvas rect; each decoration draws
    // inside it and shrinks the remaining area
    Rectangle remaining(0, 0, _chromeWidth, _chromeHeight);

    for (Size i = 0; i < _decorationCount; ++i) {
      if (_maximized && _decorations[i] == _borderBezel) continue;

      _decorations[i]->Draw(chromeCanvas, remaining);

      // shrink remaining by what this decoration consumed:
      // ExpandBounds tells us how much it would expand outward,
      // so the inverse shrinks inward
      Rectangle zero(0, 0, 0, 0);
      Rectangle expanded = _decorations[i]->ExpandBounds(zero);

      Int16 consumedLeft = static_cast<Int16>(-expanded.Origin.X);
      Int16 consumedTop = static_cast<Int16>(-expanded.Origin.Y);
      UInt16 consumedW = expanded.Dimensions.Width;
      UInt16 consumedH = expanded.Dimensions.Height;

      remaining = Rectangle(
        static_cast<Int16>(remaining.Origin.X + consumedLeft),
        static_cast<Int16>(remaining.Origin.Y + consumedTop),
        static_cast<UInt16>(
          remaining.Dimensions.Width - consumedW
        ),
        static_cast<UInt16>(
          remaining.Dimensions.Height - consumedH
        )
      );
    }

    // punch the remaining content area to transparent so the
    // client content buffer shows through
    chromeCanvas.GetPainter().FillRectangle(
      remaining, 0x00000000
    );
  }
}
