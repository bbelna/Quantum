/**
 * @file Servers/App/Windows/WindowTitleBarButton.cpp
 * @brief Implements @ref @QAppSrv::WindowTitleBarButton.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "WindowTitleBarButton.hpp"

namespace Quantum::Servers::App::Windows {
  WindowTitleBarButton::WindowTitleBarButton(
    Rectangle* windowFrame,
    UInt32 color,
    UInt32 pressedColor,
    UInt16 width,
    UInt16 height
  ) :
    _windowFrame(windowFrame),
    _color(color),
    _pressedColor(pressedColor),
    _width(width),
    _height(height)
  {}

  Rectangle WindowTitleBarButton::GetFrame() const {
    Int16 buttonY = static_cast<Int16>(
      _windowFrame->Origin.Y
      + _edgeInsetTop
      + (_contentAreaHeight - _height + 1) / 2
    );

    Int16 buttonX = static_cast<Int16>(
      _windowFrame->GetRight()
      - _edgeInsetRight
      - _offsetFromRight
      - _width
    );

    return Rectangle(buttonX, buttonY, _width, _height);
  }
}
