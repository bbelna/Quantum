/**
 * @file UI/ClickTarget.cpp
 * @brief Implements the `ClickTarget` base class.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/UI/ClickTarget.hpp>

namespace Quantum::UI {
  ClickTarget::ClickTarget(
    Int16 x,
    Int16 y,
    UInt16 width,
    UInt16 height
  ) :
    _x(x),
    _y(y),
    _width(width),
    _height(height)
  {
  }

  bool ClickTarget::HitTest(
    Int16 px,
    Int16 py
  ) const {
    return
      px >= _x &&
      px < _x + _width &&
      py >= _y &&
      py < _y + _height;
  }

  bool ClickTarget::HandleMouseDown(
    Int16 x,
    Int16 y
  ) {
    if (HitTest(x, y)) {
      _captured = true;

      OnPressedChanged(true);

      return true;
    } else {
      return false;
    }
  }

  bool ClickTarget::HandleMouseUp(
    Int16 x,
    Int16 y
  ) {
    if (!_captured) return false;

    _captured = false;

    OnPressedChanged(false);

    if (HitTest(x, y)) {
      if (_onClick) {
        _onClick(_onClickUserData);
      }

      return true;
    } else {
      return false;
    }
  }

  void ClickTarget::HandleMouseMove(
    Int16 x,
    Int16 y
  ) {
    if (_captured) {
      bool over = HitTest(x, y);

      OnPressedChanged(over);
    }
  }

  void ClickTarget::SetOnClick(
    ClickCallback callback,
    void* userData
  ) {
    _onClick = callback;
    _onClickUserData = userData;
  }
}
