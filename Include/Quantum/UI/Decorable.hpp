/**
 * @file Include/Quantum/UI/Decorable.hpp
 * @brief Declares @ref @QUI::Decorable.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Canvas.hpp"
#include "Drawable.hpp"
#include "IDecoration.hpp"
#include "UIConstants.hpp"

namespace Quantum::UI {
  /**
   * @brief Wraps a @ref Drawable and composites @ref IDecoration
   *        decorations around it.
   *
   * Decorations are rendered in the order they were added, then the inner
   * @ref IDrawable is rendered last. @ref GetBounds chains each decoration's
   * @ref IDecoration::ExpandBounds outward from the inner bounding
   * @ref Rectangle.
   */
  class Decorable : public Drawable {
    public:
      /**
       * @brief Constructs a @ref Decorable around the given @ref Drawable.
       * @param canvas Reference to the @ref Canvas to render onto.
       * @param inner Reference to the @ref Drawable to decorate.
       *              Must outlive the @ref Decorable.
       */
      Decorable(
        Canvas& canvas,
        Drawable& inner
      );

      /**
       * @brief Draws all decorations in order, then the inner drawable.
       */
      void Draw() override;

      /**
       * @brief Gets the inner bounding @ref Rectangle expanded outward through
       *        each @ref IDecoration @ref IDecoration::ExpandBounds.
       * @return The expanded bounding @ref Rectangle.
       */
      Rectangle GetBounds() const override;

      /**
       * @brief Appends a decoration to the draw list.
       * @param decoration The decoration to add. Must outlive the
       *        decorator.
       */
      void AddDecoration(IDecoration& decoration);

      /**
       * @brief Gets the inner @ref Drawable.
       * @return Reference to the inner @ref Drawable.
       */
      Drawable& GetInner() {
        return _inner;
      }

      /**
       * @brief Gets the inner @ref Drawable as a `const`.
       * @return `const` reference to the inner @ref Drawable.
       */
      const Drawable& GetInner() const {
        return _inner;
      }

    private:
      /**
       * @brief Reference to the @ref Canvas to render onto.
       * @note Not owned by this class.
       */
      Canvas& _canvas;

      /**
       * @brief Reference to the inner @ref Drawable to decorate.
       * @note Not owned by this class.
       */
      Drawable& _inner;

      /**
       * @brief Array of pointers to the decorations to apply, in order.
       */
      IDecoration* _decorations[MaxDecorations] = {};

      /**
       * @brief Number of decorations currently in the @ref _decorations array.
       */
      Size _decorationCount = 0;
  };
}
