/**
 * @file Include/Quantum/UI/UIConstants.hpp
 * @brief Defines constants for @ref @QUI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

/**
 * @brief Set to 0 to disable synthetic bold rendering globally.
 *        Useful for fonts like Chicago that are already bold.
 */
#define QUANTUM_UI_SYNTHETIC_BOLD_ENABLED 0

namespace Quantum::UI {
  static constexpr Size MaxDecorations = 8;

  /**
   * @brief Selects which corners to round in a rounded rectangle operation.
   */
  enum class RoundedCorners : UInt8 {
    /**
     * @brief No corners are rounded (equivalent to a regular rectangle).
     */
    None = 0,

    /**
     * @brief Round the top-left corner.
     */
    TopLeft = 1 << 0,

    /**
     * @brief Round the top-right corner.
     */
    TopRight = 1 << 1,

    /**
     * @brief Round the bottom-left corner.
     */
    BottomLeft = 1 << 2,

    /**
     * @brief Round the bottom-right corner.
     */
    BottomRight = 1 << 3,

    /**
     * @brief Round all four corners.
     */
    All = 0x0F,
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::UI,
  RoundedCorners
)
