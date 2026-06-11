/**
 * @file Include/Quantum/Geometry2D/Dimensions.hpp
 * @brief Declares the `DimensionsT` template and `Dimensions` alias.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Geometry2D {
  /**
   * @brief Represents a 2D extent (width and height), parameterized by
   *        dimension type.
   * @tparam T The dimension type (e.g. `UInt16`, `UInt32`).
   *
   * Dimensions are always non-negative; a zero width or height indicates an
   * empty extent with no area.
   */
  template <typename T>
  struct DimensionsT {
    /**
     * @brief The horizontal extent in pixels.
     */
    T Width;

    /**
     * @brief The vertical extent in pixels.
     */
    T Height;

    /**
     * @brief Constructs a zero extent (0 x 0).
     */
    constexpr DimensionsT() : Width(0), Height(0) {}

    /**
     * @brief Constructs an extent with the specified dimensions.
     * @param width The width in pixels.
     * @param height The height in pixels.
     */
    constexpr DimensionsT(T width, T height) : Width(width), Height(height) {}

    /**
     * @brief Returns `true` if either dimension is zero, indicating that the
     *        extent has no area.
     * @return `true` if the extent is empty.
     */
    constexpr bool IsEmpty() const {
      return Width == 0 || Height == 0;
    }

    /**
     * @brief Tests equality with another extent.
     * @param other The extent to compare.
     * @return `true` if both dimensions are equal.
     */
    constexpr bool operator==(DimensionsT other) const {
      return Width == other.Width && Height == other.Height;
    }

    /**
     * @brief Tests inequality with another extent.
     * @param other The extent to compare.
     * @return `true` if either dimension differs.
     */
    constexpr bool operator!=(DimensionsT other) const {
      return !(*this == other);
    }
  };

  /**
   * @brief A 2D extent with 16-bit unsigned dimensions.
   */
  using Dimensions = DimensionsT<UInt16>;
}
