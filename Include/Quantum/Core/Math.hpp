/**
 * @file Include/Quantum/Core/Math.hpp
 * @brief Declaration of the math namespace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

/**
 * @brief Mathematical utilities and types.
 */
namespace Quantum::Core::Math {
  /**
   * @brief Represents a range between two values.
   * @tparam ValueType
   *   The type of the start and end values. Defaults to `UInt32`.
   */
  template <typename ValueType = UInt32>
  struct Interval {
    /**
     * @brief Start value of the interval.
     */
    ValueType Start;

    /**
     * @brief End value of the interval.
     */
    ValueType End;
  };

  /**
   * @brief Integer square root via Newton's method (floor).
   * @param n The value to take the square root of.
   * @return The floor of the square root.
   */
  static inline UInt16 IntegerSqrt(UInt32 n) {
    if (n < 2) return static_cast<UInt16>(n);

    UInt32 x = n;
    UInt32 y = (x + 1) / 2;

    while (y < x) {
      x = y;
      y = (x + n / x) / 2;
    }

    return static_cast<UInt16>(x);
  }

  /**
   * @brief Computes the horizontal inset for a rounded corner at a
   *        given row. Directly tests each pixel against the circle
   *        equation `(2x - 2R + 1)^2 + (2y - 2R + 1)^2 <= (2R)^2`
   *        using an iterative scan from the outer edge inward. This
   *        avoids sqrt precision issues and produces pixel-perfect
   *        curves.
   * @param radius The corner radius in pixels.
   * @param localY The row index from the corner edge (0 = outermost).
   * @return The number of pixels to skip from the corner edge.
   */
  static inline Int16 CornerInset(UInt8 radius, UInt16 localY) {
    if (localY >= radius) return static_cast<Int16>(0);

    Int32 r2 = static_cast<Int32>(4) * radius * radius;
    Int32 dyTerm
      = static_cast<Int32>(2 * static_cast<Int32>(localY))
      - 2 * static_cast<Int32>(radius) + 1;
    Int32 dy2 = dyTerm * dyTerm;

    // scan from the outer edge inward, find the first pixel that is
    // inside the circle
    for (UInt8 x = 0; x < radius; ++x) {
      Int32 dxTerm
        = 2 * static_cast<Int32>(x)
        - 2 * static_cast<Int32>(radius) + 1;

      if (dxTerm * dxTerm + dy2 <= r2) {
        return static_cast<Int16>(x);
      }
    }

    return static_cast<Int16>(radius);
  }

  /**
   * @brief Computes antialiased coverage for a pixel in a rounded corner
   *        quadrant.
   * @param radius The corner radius in pixels.
   * @param localX Column index from the corner edge (0 = outermost).
   * @param localY Row index from the corner edge (0 = outermost).
   * @return Coverage value 0-255 (0 = fully outside, 255 = fully inside).
   *
   * Uses 128th-of-pixel precision with a 1px transition zone centered
   * on the circle edge for crisp antialiasing. Pure integer math.
   */
  static inline UInt8 CornerPixelCoverage(
    UInt8 radius, UInt16 localX, UInt16 localY
  ) {
    Int32 r128 = static_cast<Int32>(radius) << 7;
    Int32 dx = (static_cast<Int32>(localX) << 7) + 64 - r128;
    Int32 dy = (static_cast<Int32>(localY) << 7) + 64 - r128;

    UInt32 dist2 = static_cast<UInt32>(dx * dx)
      + static_cast<UInt32>(dy * dy);

    Int32 innerR = r128 - 64;

    if (innerR > 0) {
      UInt32 innerR2
        = static_cast<UInt32>(innerR) * static_cast<UInt32>(innerR);

      if (dist2 <= innerR2) return 255;
    }

    UInt32 outerR = static_cast<UInt32>(r128 + 64);
    UInt32 outerR2 = outerR * outerR;

    if (dist2 >= outerR2) return 0;

    Int32 dist = static_cast<Int32>(IntegerSqrt(dist2));
    Int32 signedDist = dist - r128;
    Int32 coverage = ((64 - signedDist) * 255 + 64) >> 7;

    if (coverage < 0) return 0;
    if (coverage > 255) return 255;

    return static_cast<UInt8>(coverage);
  }


  template <typename ValueType = UInt32>
  ValueType Clamp(ValueType value, ValueType min, ValueType max) {
    if (value < min) {
      return min;
    } else if (value > max) {
      return max;
    } else {
      return value;
    }
  }
}
