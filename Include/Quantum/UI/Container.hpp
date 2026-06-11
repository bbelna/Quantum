/**
 * @file Include/Quantum/UI/Frame.hpp
 * @brief Declares @ref @QUI::Frame.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "Inset.hpp"
#include "Rectangle.hpp"

namespace Quantum::UI {
  /**
   * @brief @ref Frame::Rectangle with @ref Frame::Padding and
   *        @ref Frame::Margin.
   */
  struct Container {
    Rectangle Bounds;

    Inset Padding;

    Inset Margin;
  };
}
