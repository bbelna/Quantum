/**
 * @file Servers/App/Windows/WindowTitleBar.hpp
 * @brief Declares @ref @QAppSrv::Windows::WindowTitleBar.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>
#include <Quantum/UI/Bezel.hpp>

#include "WindowTheme.hpp"
#include "WindowTitleBarButtonBitmaps.hpp"
#include "WindowTitleBarButtons.hpp"

namespace Quantum::Servers::App::Windows {
  /**
   * @brief Computes the title bar height from the theme padding and
   *        the tallest interior content (buttons or font glyphs).
   */
  inline UInt16 ComputeTitleBarHeight(
    const WindowTitleBarTheme& theme,
    const Fonts::BitmapFont* font
  ) {
    UInt16 contentHeight = WindowTitleBarButtonBitmaps::ButtonHeight;

    if (font && font->Height > contentHeight) {
      contentHeight = font->Height;
    }

    return static_cast<UInt16>(
      theme.Padding.Top + contentHeight + theme.Padding.Bottom
    );
  }

  /**
   * @brief The window title bar — an @ref IDecoration that renders the
   *        title bar background, text, and traffic-light buttons into
   *        the chrome canvas, and provides hit-testing for the title
   *        bar region and its buttons.
   */
  class WindowTitleBar : public IDecoration {
    public:
      /**
       * @brief Constructs a title bar bound to its owning window's
       *        frame rectangle.
       * @param theme Title bar theme reference. Must outlive this.
       * @param windowFrame Pointer to the owning window's outer frame.
       * @param bezelTotal Total bezel inset from frame edge.
       * @param allowClose Whether the close button is present.
       * @param allowMaximize Whether maximize/minimize buttons are
       *                      present.
       */
      WindowTitleBar(
        const WindowTitleBarTheme& theme,
        Rectangle* windowFrame,
        UInt16 bezelTotal,
        const Fonts::BitmapFont* font,
        bool allowClose,
        bool allowMaximize
      );

      ~WindowTitleBar();

      // ── IDecoration ───────────────────────────────────────────────

      void Draw(Canvas& canvas, Rectangle contentRectangle) override;

      Rectangle ExpandBounds(
        Rectangle contentRectangle
      ) const override;

      // ── state ─────────────────────────────────────────────────────

      void SetActive(bool active);
      void SetMaximized(bool maximized);
      void SetTitle(const char* title);
      void SetFont(const Fonts::BitmapFont* font) { _font = font; }

      // ── hit testing (screen-space, from owning window's frame) ────

      /**
       * @brief Returns the title bar's screen-space rectangle.
       */
      Rectangle GetFrame() const;

      /**
       * @brief Returns the button container for per-button hit testing.
       */
      WindowTitleBarButtons* GetButtons() const { return _buttons; }

      /**
       * @brief Returns the computed title bar height.
       */
      UInt16 GetHeight() const;

      // ── button accessors ──────────────────────────────────────────

      WindowTitleBarButton* GetCloseButton() const {
        return _closeButton;
      }

      WindowTitleBarButton* GetMaximizeButton() const {
        return _maximizeButton;
      }

      WindowTitleBarButton* GetMinimizeButton() const {
        return _minimizeButton;
      }

    private:
      const WindowTitleBarTheme& _theme;
      Rectangle* _windowFrame;
      UInt16 _bezelTotal;
      WindowTitleBarButtons* _buttons;
      WindowTitleBarButton* _closeButton = nullptr;
      WindowTitleBarButton* _maximizeButton = nullptr;
      WindowTitleBarButton* _minimizeButton = nullptr;
      const Fonts::BitmapFont* _font = nullptr;
      char _title[64] = {};
      bool _active = true;
      bool _maximized = false;
      UI::Bezel _bezel;

      void _renderButton(
        Canvas& canvas,
        Int16 x,
        Int16 y,
        UInt32 color
      ) const;
  };
}
