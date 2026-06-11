/**
 * @file Include/Quantum/UI/Drawable.hpp
 * @brief Declares @ref @QUI::Drawable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Rectangle.hpp"

namespace Quantum::UI {
  /**
   * @brief Universal base for all drawable UI objects.
   *
   * Every entity in the UI hierarchy — canvases, elements, decorators —
   * implements this interface. It provides a common type for composition
   * and decoration without coupling concrete types to one another.
   */
  class Drawable {
    public:
      virtual ~Drawable() = default;

      /**
       * @brief Renders this drawable.
       */
      virtual void Draw() = 0;

      /**
       * @brief Returns the bounding rectangle of this drawable in
       *        content-pixel coordinates.
       */
      virtual Rectangle GetBounds() const = 0;
  };
}
