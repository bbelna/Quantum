/**
 * @file Include/Quantum/UI/Rectangle.hpp
 * @brief Declares @ref @QUI::Rectangle.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Geometry2D/Rectangle.hpp>

namespace Quantum::UI {
  /**
   * @brief @ref Geometry2D::RectangleT with @ref Int16 coordinates.
   *
   * This type is used throughout @ref @QUI to define bounding rectangles.
   */
  using Rectangle = Geometry2D::RectangleT<Int16, UInt16>;
}
