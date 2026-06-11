/**
 * @file Include/Quantum/Graphics/PixelFormat.hpp
 * @brief Declares @ref @QGFX::PixelFormat.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Graphics {
  /**
   * @brief Identifies the layout of a single pixel.
   *
   * A @ref PixelFormat names the channel layout and bit depth of a
   * pixel. It is deliberately independent of the byte size of a pixel
   * so that formats sharing the same size (e.g. `RGB555` and `RGB565`)
   * can be distinguished. Use @ref BytesPerPixel and @ref BitsPerPixel
   * to query size information.
   */
  enum class PixelFormat : UInt8 {
    /**
     * @brief 16 bits per pixel (5 bits red, 6 bits green, 5 bits blue,
     *        no alpha).
     */
    RGB565 = 0,

    /**
     * @brief 32 bits per pixel (8 bits alpha, 8 bits red, 8 bits green,
     *        8 bits blue).
     */
    ARGB32 = 1,
  };

  /**
   * @brief Returns the number of bytes occupied by a single pixel of
   *        the given @ref PixelFormat.
   * @param format The @ref PixelFormat to query.
   * @return The number of bytes per pixel, or `0` if @p format is not
   *         recognized.
   */
  constexpr UInt8 BytesPerPixel(PixelFormat format) {
    switch (format) {
      case PixelFormat::RGB565: {
        return 2;
      }

      case PixelFormat::ARGB32: {
        return 4;
      }

      default: {
        return 0;
      }
    }
  }

  /**
   * @brief Returns the number of bits occupied by a single pixel of
   *        the given @ref PixelFormat.
   * @param format The @ref PixelFormat to query.
   * @return The number of bits per pixel, or `0` if @p format is not
   *         recognized.
   */
  constexpr UInt8 BitsPerPixel(PixelFormat format) {
    return static_cast<UInt8>(BytesPerPixel(format) * 8);
  }
}
