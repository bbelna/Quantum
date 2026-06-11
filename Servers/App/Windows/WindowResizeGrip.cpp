/**
 * @file Servers/App/Windows/WindowResizeGrip.cpp
 * @brief Implements @ref @QAppSrv::WindowResizeGrip.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "WindowResizeGrip.hpp"

#include <Quantum/UI/Canvas.hpp>

namespace Quantum::Servers::App::Windows {
  WindowResizeGrip::WindowResizeGrip(
    const ResizeGripTheme& theme,
    Rectangle* windowFrame
  ) :
    _theme(theme),
    _windowFrame(windowFrame)
  {}

  Rectangle WindowResizeGrip::GetFrame() const {
    return Rectangle(
      static_cast<Int16>(_windowFrame->GetRight() - _theme.HitSize),
      static_cast<Int16>(_windowFrame->GetBottom() - _theme.HitSize),
      _theme.HitSize,
      _theme.HitSize
    );
  }

  void WindowResizeGrip::Draw(
    Canvas& canvas,
    Rectangle contentRectangle
  ) {
    if (!_enabled) return;

    Painter& painter = canvas.GetPainter();

    Int16 gripRight = static_cast<Int16>(
      contentRectangle.GetRight() - _theme.DotSize - 2
    );
    Int16 gripBottom = static_cast<Int16>(
      contentRectangle.GetBottom() - _theme.DotSize - 2
    );

    auto drawDot = [&](Int16 x, Int16 y) {
      painter.FillRectangle(
        Rectangle(x, y, _theme.DotSize, _theme.DotSize),
        _theme.DotColor
      );
    };

    drawDot(gripRight, gripBottom);
    drawDot(
      static_cast<Int16>(gripRight - _theme.DotGap),
      gripBottom
    );
    drawDot(
      gripRight,
      static_cast<Int16>(gripBottom - _theme.DotGap)
    );
  }

  Rectangle WindowResizeGrip::ExpandBounds(
    Rectangle contentRectangle
  ) const {
    return contentRectangle;
  }
}
