/**
 * @file Include/Quantum/Geometry2D/Point.hpp
 * @brief Declares the `PointT` template and `Point` alias for 2D coordinates.
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
   * @brief Represents a point in 2D space.
   * @tparam T The coordinate type (e.g. `Int16`, `Int32`).
   */
  template <typename T>
  struct PointT {
    /**
     * @brief The X (horizontal) coordinate.
     */
    T X;

    /**
     * @brief The Y (vertical) coordinate.
     */
    T Y;

    /**
     * @brief Constructs a point at the origin (0, 0).
     */
    constexpr PointT() : X(0), Y(0) {}

    /**
     * @brief Constructs a point at the specified coordinates.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     */
    constexpr PointT(T x, T y) : X(x), Y(y) {}

    /**
     * @brief Returns the sum of this point and another.
     * @param other The point to add.
     * @return A new point whose coordinates are the element-wise sum.
     */
    constexpr PointT operator+(PointT other) const {
      return { static_cast<T>(X + other.X), static_cast<T>(Y + other.Y) };
    }

    /**
     * @brief Returns the difference of this point and another.
     * @param other The point to subtract.
     * @return A new point whose coordinates are the element-wise difference.
     */
    constexpr PointT operator-(PointT other) const {
      return { static_cast<T>(X - other.X), static_cast<T>(Y - other.Y) };
    }

    /**
     * @brief Adds another point to this point in place.
     * @param other The point to add.
     * @return Reference to this point after addition.
     */
    constexpr PointT& operator+=(PointT other) {
      X = static_cast<T>(X + other.X);
      Y = static_cast<T>(Y + other.Y);

      return *this;
    }

    /**
     * @brief Subtracts another point from this point in place.
     * @param other The point to subtract.
     * @return Reference to this point after subtraction.
     */
    constexpr PointT& operator-=(PointT other) {
      X = static_cast<T>(X - other.X);
      Y = static_cast<T>(Y - other.Y);

      return *this;
    }

    /**
     * @brief Tests equality with another point.
     * @param other The point to compare.
     * @return `true` if both coordinates are equal.
     */
    constexpr bool operator==(PointT other) const {
      return X == other.X && Y == other.Y;
    }

    /**
     * @brief Tests inequality with another point.
     * @param other The point to compare.
     * @return `true` if either coordinate differs.
     */
    constexpr bool operator!=(PointT other) const {
      return !(*this == other);
    }
  };

  /**
   * @brief A 2D point with 16-bit signed coordinates.
   */
  using Point = PointT<Int16>;
}
