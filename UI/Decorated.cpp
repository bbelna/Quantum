/**
 * @file UI/Decorable.cpp
 * @brief Implements @ref @QUI::Decorable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/UI/Decorable.hpp>

#include "UITypes.hpp"

namespace Quantum::UI {
  Decorable::Decorable(
    Canvas& canvas,
    Drawable& inner
  ) :
    _canvas(canvas),
    _inner(inner)
  {
  }

  void Decorable::Draw() {
    Rectangle contentBounds = _inner.GetBounds();

    for (Size i = 0; i < _decorationCount; ++i) {
      _decorations[i]->Draw(_canvas, contentBounds);
    }

    _inner.Draw();
  }

  Rectangle Decorable::GetBounds() const {
    Rectangle bounds = _inner.GetBounds();

    for (Size i = 0; i < _decorationCount; ++i) {
      bounds = _decorations[i]->ExpandBounds(bounds);
    }

    return bounds;
  }

  void Decorable::AddDecoration(IDecoration& decoration) {
    if (_decorationCount < MaxDecorations) {
      _decorations[_decorationCount] = &decoration;
      ++_decorationCount;
    }
  }
}
