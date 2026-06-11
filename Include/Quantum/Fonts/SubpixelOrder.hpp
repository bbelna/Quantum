/**
 * @file Include/Quantum/Fonts/SubpixelOrder.hpp
 * @brief Declares @ref @QFont::SubpixelOrder.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Fonts {
  /**
   * @brief Subpixel layout order for LCD-rendered fonts.
   */
  enum class SubpixelOrder : UInt8 {
    /**
     * @brief No subpixel rendering (grayscale or monochrome).
     */
    None = 0,

    /**
     * @brief Red-Green-Blue (RGB) subpixel order.
     */
    RGB = 1,

    /**
     * @brief Blue-Green-Red (BGR) subpixel order.
     */
    BGR = 2,
  };
}
