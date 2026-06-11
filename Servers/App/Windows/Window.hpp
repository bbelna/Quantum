/**
 * @file Servers/App/Windows/Window.hpp
 * @brief Declares @ref @QAppSrv::Window.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "DefaultWindowTheme.hpp"
#include "WindowResizeGrip.hpp"
#include "WindowTitleBar.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Represents a composited window with theme-driven decorations
   *        rendered into a pre-rendered chrome canvas.
   *
   * The window border is a @ref UI::Bezel. The title bar and resize
   * grip are @ref IDecoration instances that also provide hit testing.
   * All decorations render into a private ARGB32 chrome buffer that is
   * blitted through @ref DrawContext::BlitBuffer at composite time.
   */
  class Window {
    public:
      static void SetTitleFont(const Fonts::BitmapFont* font);
      static const Fonts::BitmapFont& GetTitleFont();

      static UInt16 GetTitleBarHeight(const WindowTheme& theme);

      static UInt16 ContentWidthFromChromeWidth(
        UInt16 chromeWidth,
        bool maximized,
        const WindowTheme& theme
      );

      static UInt16 ContentHeightFromChromeHeight(
        UInt16 chromeHeight,
        bool maximized,
        const WindowTheme& theme
      );

      static UInt16 EffectiveOuterInset(
        bool maximized,
        const WindowTheme& theme
      );

      Window(
        Int16 x,
        Int16 y,
        UInt16 width,
        UInt16 height,
        const char* title,
        const WindowTheme& theme,
        bool allowClose = true,
        bool allowMaximize = true,
        bool allowResize = true,
        bool chromeless = false
      );

      ~Window();

      bool IsChromeless() const { return _chromeless; }
      const WindowTheme& GetTheme() const { return _theme; }
      Rectangle GetFrame() const { return _frame; }
      Rectangle GetChromeFrame() const;
      Rectangle GetContentFrame() const { return _getContentFrame(); }

      Int16 GetX() const { return _frame.Origin.X; }
      Int16 GetY() const { return _frame.Origin.Y; }
      UInt16 GetWidth() const { return _frame.Dimensions.Width; }
      UInt16 GetHeight() const { return _frame.Dimensions.Height; }
      const char* GetTitle() const { return _title; }
      UInt16 GetButtonMarginWidth() const;

      void SetPosition(Int16 x, Int16 y);
      void SetSize(UInt16 width, UInt16 height);

      bool HitTest(Int16 px, Int16 py) const;
      bool HitTestTitleBar(Int16 px, Int16 py) const;
      bool HitTestCloseButton(Int16 px, Int16 py) const;
      bool HitTestMaximizeButton(Int16 px, Int16 py) const;
      bool HitTestMinimizeButton(Int16 px, Int16 py) const;
      bool HitTestResizeHandle(Int16 px, Int16 py) const;

      WindowTitleBarButton* GetCloseButton() const;
      WindowTitleBarButton* GetMaximizeButton() const;
      WindowTitleBarButton* GetMinimizeButton() const;

      bool IsMinimized() const { return _minimized; }
      void SetMinimized(bool minimized) { _minimized = minimized; }
      bool IsContentReady() const { return _contentReady; }
      void SetContentReady(bool ready) { _contentReady = ready; }
      bool IsMaximized() const { return _maximized; }

      void SetMaximized(
        bool maximized,
        UInt16 screenWidth,
        UInt16 screenHeight,
        UInt16 topOffset = 0,
        UInt16 bottomOffset = 0
      );

      void DrawClipped(
        Rectangle clipRectangle,
        const DrawContext& context
      ) const;

      void SetContentBuffer(
        void* buffer,
        UInt16 width,
        UInt16 height,
        UInt16 stride,
        UInt8 bytesPerPixel
      );

      void SetContentColor(UInt32 color) { _contentColor = color; }
      UInt32 GetContentColor() const { return _contentColor; }

      void SetInnerContentColor(UInt32 color) {
        _innerContentColor = color;
      }

      UInt32 GetInnerContentColor() const { return _innerContentColor; }

      void SetActive(bool active);
      bool IsActive() const { return _active; }

      /**
       * @brief Returns the visual bounds (identical to frame since
       *        shadows are not supported).
       */
      Rectangle GetVisualBounds() const { return _frame; }

      /**
       * @brief Returns the total inset from the frame edge to the
       *        content area contributed by all active decorations.
       *        Computed by chaining @ref IDecoration::ExpandBounds.
       */
      UInt16 GetDecorationThickness() const;

    private:
      const WindowTheme& _theme;
      Rectangle _frame;
      char _title[64];

      // ── components ────────────────────────────────────────────────
      WindowTitleBar* _titleBar = nullptr;
      WindowResizeGrip* _resizeGrip = nullptr;
      UI::Bezel* _borderBezel = nullptr;

      // ── decoration list ───────────────────────────────────────────
      IDecoration* _decorations[MaxDecorations] = {};
      Size _decorationCount = 0;
      Size _decorationsDrawn = 0;

      // ── chrome buffer ─────────────────────────────────────────────
      UInt32* _chromeBuffer = nullptr;
      UInt16 _chromeWidth = 0;
      UInt16 _chromeHeight = 0;
      Size _chromeBufferCapacity = 0;

      // ── state ─────────────────────────────────────────────────────
      bool _maximized = false;
      bool _minimized = false;
      bool _contentReady = false;
      bool _allowResize = true;
      bool _chromeless = false;
      bool _active = true;
      Rectangle _restoreFrame;

      // ── content buffer ────────────────────────────────────────────
      void* _contentBuffer = nullptr;
      UInt8 _contentBPP = 4;
      UInt16 _contentWidth = 0;
      UInt16 _contentHeight = 0;
      UInt16 _contentStride = 0;
      UInt32 _contentColor;
      UInt32 _innerContentColor = 0;

      void _rebuildChrome();
      Rectangle _getContentFrame() const;
      UInt16 _getOuterInset() const;
      UInt16 _getTitleBarHeight() const;
  };
}
