/**
 * @file Servers/App/Dock/Dock.hpp
 * @brief Declares @ref @QAppSrv::Dock::Dock.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "DockButton.hpp"
#include "DockConstants.hpp"

namespace Quantum::Servers::App::Dock {
  /**
   * @brief Displays a list of applications/windows and provides window
   *        minimize functionality.
   */
  class Dock {
    public:
      /**
       * @brief Creates a new @ref Dock instance.
       */
      Dock() = default;

      void Init(
        WindowManager* windowManager,
        UInt16 screenWidth,
        UInt16 screenHeight,
        const Fonts::BitmapFont* activeFont,
        const Fonts::BitmapFont* inactiveFont
      );

      UInt16 GetHeight() const { return _height; }
      Rectangle GetFrame() const;
      bool Contains(Int16 screenX, Int16 screenY) const;
      UInt32 HitTest(Int16 screenX, Int16 screenY) const;
      void SetDirty() { _dirty = true; }
      bool IsDirty() const { return _dirty; }
      void Rebuild();
      void DrawClipped(Rectangle clipRect, const DrawContext& ctx) const;

    private:
      WindowManager* _windowManager = nullptr;
      const Fonts::BitmapFont* _activeFont = nullptr;
      const Fonts::BitmapFont* _inactiveFont = nullptr;
      UInt16 _screenWidth = 0;
      UInt16 _screenHeight = 0;
      UInt16 _height = 0;
      UInt16 _buttonHeight = ButtonFallbackHeight;
      volatile bool _dirty = true;
      DockButton _buttons[MaxButtons] = {};
      Size _buttonCount = 0;
      bool _isFull = false;
      UInt32* _surface = nullptr;
      UInt16 _surfaceWidth = 0;
      UInt16 _surfaceHeight = 0;

      void _renderSurface();
  };
}
