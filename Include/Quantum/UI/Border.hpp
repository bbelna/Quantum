/**
 * @file Include/Quantum/UI/Border.hpp
 * @brief Declares @ref @QUI::Border.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Canvas.hpp"
#include "Container.hpp"
#include "IDecoration.hpp"
#include "Rectangle.hpp"

namespace Quantum::UI {
  /**
   * @brief A rectangular border decoration with per-side colors and
   *        thicknesses.
   *
   * Implements @ref IDecoration so it can be added to a @ref Decorable
   * compositor. Draws four edge strips around the content bounds.
   */
  struct Border : public Container, public IDecoration {
    FrameT<UInt32> Colors = { 0, 0, 0, 0 };

    FrameT<UInt8> Thicknesses = { 0, 0, 0, 0 };

    /**
     * @brief Draws the border edges around the content region.
     * @param canvas The canvas to draw on.
     * @param contentBounds The inner drawable's bounding rectangle.
     */
    void Draw(
      Canvas& canvas,
      Rectangle contentBounds
    ) override;

    /**
     * @brief Expands bounds outward by the per-side thicknesses.
     * @param contentBounds The inner drawable's bounding rectangle.
     * @return The expanded rectangle including the border.
     */
    Rectangle ExpandBounds(
      Rectangle contentBounds
    ) const override;
  };
}
