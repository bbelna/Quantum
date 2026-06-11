/**
 * @file Include/Quantum/Core/Align.hpp
 * @brief Declaration of alignment utilities.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

/**
 * @brief Aligns a value up to the nearest multiple of alignment.
 * @tparam ValueType The type of the value.
 * @param value The value to align.
 * @param alignment The alignment boundary.
 * @return The aligned value.
 */
template <typename ValueType>
ValueType AlignUp(ValueType value, Size alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Aligns a value down to the nearest multiple of alignment.
 * @tparam ValueType The type of the value.
 * @param value The value to align.
 * @param alignment The alignment boundary.
 * @return The aligned value.
 */
template <typename ValueType>
ValueType AlignDown(ValueType value, Size alignment) {
  return value & ~(alignment - 1);
}
