/**
 * @file UI/Element.cpp
 * @brief Implements @ref @QUI::Element.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/App/Window.hpp>
#include <Quantum/UI/Element.hpp>

#include "UITypes.hpp"

namespace Quantum::UI {
  Element::~Element() {
    if (_window) {
      _window->Remove(*this);
    }
  }

  Rectangle Element::GetBounds() const {
    return Rectangle(
      0,
      0,
      _canvas.GetWidth(),
      _canvas.GetHeight()
    );
  }

  void Element::Invalidate() {
    if (_window) {
      _window->Invalidate(GetBounds());
    }
  }
}
