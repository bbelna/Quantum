/**
 * @file Include/Quantum/UI/IDecoration.hpp
 * @brief Declares @ref @QUI::IDecoration.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Rectangle.hpp"

namespace Quantum::UI {
  class Canvas;

  /**
   * @brief Interface for decoration strategies.
   * @see Decorable
   *
   * Each decoration type implements this interface to encapsulate its own
   * rendering logic. A @ref Decorable invokes decorations in order, passing the
   * inner @ref Drawable @ref  as the content region.
   */
  class IDecoration {
    public:
      virtual ~IDecoration() = default;

      /**
       * @brief Decorates a @p contentRectangle on the given @p canvas.
       * @param canvas The @ref Canvas to draw the @ref IDecorable on.
       * @param contentBounds
       *   The @ref IDrawable bounding @ref Rectangle to apply this
       *   @ref IDecoration to.
       */
      virtual void Draw(
        Canvas& canvas,
        Rectangle contentRectangle
      ) = 0;

      /**
       * @brief
       *   Expands @ref Rectangle bounds outward to account for this
       *   @ref IDecorable.
       * @param contentRectangle The @ref IDrawable bounding @ref Rectangle.
       * @return The expanded @ref Rectangle including this decoration.
       */
      virtual Rectangle ExpandBounds(Rectangle contentRectangle) const = 0;
  };
}
