/**
 * @file Include/Quantum/Geometry2D/HasFrame.hpp
 * @brief Declares and implements @ref Quantum::Geometry2D::HasFrame.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Point.hpp"
#include "Rectangle.hpp"

namespace Quantum::Geometry2D {
  /**
   * @brief Base class for objects that occupy a rectangular region on the
   *        screen.
   *
   * Provides a pure virtual @ref HasFrame::GetFrame accessor
   * and a concrete @ref HasFrame::Contains hit-test method.
   * Follows the @ref @QCore::HasValue pattern from
   * @ref Include/Quantum/Core/Value.hpp.
   */
  class HasFrame {
    public:
      /**
       * @brief Destroys this @ref HasFrame instance.
       */
      virtual ~HasFrame() = default;

      /**
       * @brief Returns the bounding rectangle of this frame.
       * @return This frame's rectangle.
       */
      virtual Rectangle GetFrame() const = 0;

      /**
       * @brief Tests whether a point lies within this frame.
       * @param p The point to test.
       * @return `true` if the point is inside this frame; `false` otherwise.
       */
      bool Contains(Point p) const { return GetFrame().Contains(p); }
  };
}
