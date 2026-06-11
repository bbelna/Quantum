/**
 * @file Include/Quantum/Geometry2D/Rectangle.hpp
 * @brief Declares the `RectangleT` template and `Rectangle` alias.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Dimensions.hpp"
#include "Point.hpp"

namespace Quantum::Geometry2D {
  /**
   * @brief Represents an axis-aligned rectangle in 2D space.
   * @tparam CoordT The signed coordinate type (e.g. `Int16`, `Int32`).
   * @tparam DimT The unsigned dimension type (e.g. `UInt16`, `UInt32`).
   */
  template <typename CoordT, typename DimT = CoordT>
  struct RectangleT {
    /**
     * @brief The top-left corner of the rectangle.
     */
    PointT<CoordT> Origin;

    /**
     * @brief The width and height of the rectangle.
     */
    DimensionsT<DimT> Dimensions;

    /**
     * @brief Constructs an empty rectangle at the origin (position 0,0,
     *        size 0x0).
     */
    constexpr RectangleT() : Origin(), Dimensions() {}

    /**
     * @brief Constructs a rectangle from an origin point and a dimensions
     *        extent.
     * @param origin The top-left corner of the rectangle.
     * @param dimensions The width and height of the rectangle.
     */
    constexpr RectangleT(
      PointT<CoordT> origin,
      DimensionsT<DimT> dimensions
    ) : Origin(origin), Dimensions(dimensions) {}

    /**
     * @brief Constructs a rectangle from individual coordinate and dimension
     *        values.
     * @param x The x-coordinate of the top-left corner.
     * @param y The y-coordinate of the top-left corner.
     * @param width The width of the rectangle in pixels.
     * @param height The height of the rectangle in pixels.
     */
    constexpr RectangleT(CoordT x, CoordT y, DimT width, DimT height)
      : Origin(x, y), Dimensions(width, height) {}

    /**
     * @brief Returns the x-coordinate of the left edge.
     * @return The left edge x-coordinate.
     */
    constexpr CoordT GetLeft() const {
      return Origin.X;
    }

    /**
     * @brief Returns the y-coordinate of the top edge.
     * @return The top edge y-coordinate.
     */
    constexpr CoordT GetTop() const {
      return Origin.Y;
    }

    /**
     * @brief Returns the exclusive x-coordinate of the right edge
     *        (left + width).
     * @return The right edge x-coordinate.
     */
    constexpr CoordT GetRight() const {
      return static_cast<CoordT>(Origin.X + Dimensions.Width);
    }

    /**
     * @brief Returns the exclusive y-coordinate of the bottom edge
     *        (top + height).
     * @return The bottom edge y-coordinate.
     */
    constexpr CoordT GetBottom() const {
      return static_cast<CoordT>(Origin.Y + Dimensions.Height);
    }

    /**
     * @brief Returns `true` if the rectangle has zero area (either dimension
     *        is zero).
     * @return `true` if the rectangle is empty.
     */
    constexpr bool IsEmpty() const {
      return Dimensions.IsEmpty();
    }

    /**
     * @brief Tests whether a point lies within the rectangle. The test is
     *        half-open: the left and top edges are inclusive, the right and
     *        bottom edges are exclusive.
     * @param p The point to test.
     * @return `true` if the point is inside the rectangle.
     */
    constexpr bool Contains(PointT<CoordT> p) const {
      return p.X >= Origin.X &&
             p.X < GetRight() &&
             p.Y >= Origin.Y &&
             p.Y < GetBottom();
    }

    /**
     * @brief Tests whether another rectangle is fully contained within this
     *        one. An empty rectangle is considered contained by any non-empty
     *        rectangle.
     * @param other The rectangle to test.
     * @return `true` if `other` lies entirely within this rectangle.
     */
    constexpr bool Contains(RectangleT other) const {
      return other.Origin.X >= Origin.X &&
             other.GetRight() <= GetRight() &&
             other.Origin.Y >= Origin.Y &&
             other.GetBottom() <= GetBottom();
    }

    /**
     * @brief Tests whether this rectangle overlaps with another. Two
     *        rectangles that share only an edge (zero-area overlap) are not
     *        considered to intersect.
     * @param other The rectangle to test against.
     * @return `true` if the rectangles overlap in a non-zero area.
     */
    constexpr bool Intersects(RectangleT other) const {
      return Origin.X < other.GetRight() &&
             GetRight() > other.Origin.X &&
             Origin.Y < other.GetBottom() &&
             GetBottom() > other.Origin.Y;
    }

    /**
     * @brief Returns the intersection of this rectangle and another. If the
     *        rectangles do not overlap, the returned rectangle is empty
     *        (position 0,0, size 0x0).
     * @param other The rectangle to intersect with.
     * @return The overlapping region, or an empty rectangle if none.
     */
    constexpr RectangleT Intersect(RectangleT other) const {
      CoordT x1 = Origin.X > other.Origin.X
        ? Origin.X
        : other.Origin.X;
      CoordT y1 = Origin.Y > other.Origin.Y
        ? Origin.Y
        : other.Origin.Y;
      CoordT x2 = GetRight() < other.GetRight()
        ? GetRight()
        : other.GetRight();
      CoordT y2 = GetBottom() < other.GetBottom()
        ? GetBottom()
        : other.GetBottom();

      if (x2 <= x1 || y2 <= y1) return RectangleT();

      return RectangleT(
        x1, y1,
        static_cast<DimT>(x2 - x1),
        static_cast<DimT>(y2 - y1)
      );
    }

    /**
     * @brief Returns the smallest rectangle that contains both this rectangle
     *        and another. If either operand is empty, the non-empty operand
     *        is returned unchanged.
     * @param other The rectangle to union with.
     * @return The bounding rectangle of both operands.
     */
    constexpr RectangleT Union(RectangleT other) const {
      if (IsEmpty()) return other;
      if (other.IsEmpty()) return *this;

      CoordT x1 = Origin.X < other.Origin.X
        ? Origin.X
        : other.Origin.X;
      CoordT y1 = Origin.Y < other.Origin.Y
        ? Origin.Y
        : other.Origin.Y;
      CoordT x2 = GetRight() > other.GetRight()
        ? GetRight()
        : other.GetRight();
      CoordT y2 = GetBottom() > other.GetBottom()
        ? GetBottom()
        : other.GetBottom();

      return RectangleT(
        x1,
        y1,
        static_cast<DimT>(x2 - x1),
        static_cast<DimT>(y2 - y1)
      );
    }

    /**
     * @brief Returns a copy of this rectangle shrunk by `amount` pixels on
     *        all four sides. If the rectangle is too small to inset (either
     *        dimension would become zero or negative), an empty rectangle is
     *        returned.
     * @param amount The number of pixels to inset on each edge.
     * @return The inset rectangle, or an empty rectangle if the inset exceeds
     *         the rectangle's size.
     */
    constexpr RectangleT Inset(DimT amount) const {
      DimT total = static_cast<DimT>(2 * amount);

      if (Dimensions.Width <= total || Dimensions.Height <= total)
        return RectangleT();

      return RectangleT(
        static_cast<CoordT>(Origin.X + amount),
        static_cast<CoordT>(Origin.Y + amount),
        static_cast<DimT>(Dimensions.Width - total),
        static_cast<DimT>(Dimensions.Height - total)
      );
    }

    /**
     * @brief Returns a copy of this rectangle shrunk by `dx` pixels on the
     *        left and right edges and `dy` pixels on the top and bottom
     *        edges. If the rectangle is too small to inset, an empty
     *        rectangle is returned.
     * @param dx The number of pixels to inset on the left and right edges.
     * @param dy The number of pixels to inset on the top and bottom edges.
     * @return The inset rectangle, or an empty rectangle if the inset exceeds
     *         the rectangle's size.
     */
    constexpr RectangleT Inset(DimT dx, DimT dy) const {
      DimT totalX = static_cast<DimT>(2 * dx);
      DimT totalY = static_cast<DimT>(2 * dy);

      if (Dimensions.Width <= totalX || Dimensions.Height <= totalY)
        return RectangleT();

      return RectangleT(
        static_cast<CoordT>(Origin.X + dx),
        static_cast<CoordT>(Origin.Y + dy),
        static_cast<DimT>(Dimensions.Width - totalX),
        static_cast<DimT>(Dimensions.Height - totalY)
      );
    }

    /**
     * @brief Returns a copy of this rectangle translated by the given delta.
     *        The size is unchanged.
     * @param dx The horizontal displacement in pixels.
     * @param dy The vertical displacement in pixels.
     * @return The translated rectangle.
     */
    constexpr RectangleT Translated(CoordT dx, CoordT dy) const {
      return RectangleT(
        static_cast<CoordT>(Origin.X + dx),
        static_cast<CoordT>(Origin.Y + dy),
        Dimensions.Width,
        Dimensions.Height
      );
    }

    /**
     * @brief Tests equality with another rectangle.
     * @param other The rectangle to compare.
     * @return `true` if both the origin and dimensions are equal.
     */
    constexpr bool operator==(RectangleT other) const {
      return Origin == other.Origin && Dimensions == other.Dimensions;
    }

    /**
     * @brief Tests inequality with another rectangle.
     * @param other The rectangle to compare.
     * @return `true` if either the origin or dimensions differ.
     */
    constexpr bool operator!=(RectangleT other) const {
      return !(*this == other);
    }
  };

  /**
   * @brief A rectangle with 16-bit signed coordinates and 16-bit unsigned
   *        dimensions.
   */
  using Rectangle = RectangleT<Int16, UInt16>;
}
