/**
 * @file Include/Quantum/UI/FrameT.hpp
 * @brief Declares @ref @QUI::FrameT.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::UI {
  /**
   * @brief Represents a frame with typed dimensions.
   * @tparam T The type of the frame's dimensions (e.g. @ref Size, @ref UInt32).
   * @note @ref FrameT varies from @ref Rectangle / @ref Geometry2D::RectangleT
   *       in that it only holds @ref Frame::Top, @ref Frame::Left,
   *       @ref Frame::Right, and @ref Frame::Bottom values. i.e., it defines
   *       only geometry; it does not carry a spatial properties.
   */
  template <typename T>
  struct FrameT {
    /**
     * @brief Top value of the @ref FrameT.
     */
    T Top;

    /**
     * @brief Left value of the @ref FrameT.
     */
    T Left;

    /**
     * @brief Right value of the @ref FrameT.
     */
    T Right;

    /**
     * @brief Bottom value of the @ref FrameT.
     */
    T Bottom;
  };
}
