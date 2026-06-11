/**
 * @file Include/Quantum/Core/Sort.hpp
 * @brief Declares @ref @QCore::Sort.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core::Sort {
  /**
   * @brief Sorts an array in-place using insertion sort.
   * @tparam T Element type.
   * @tparam Compare A callable with signature `bool(const T&, const T&)`
   *         that returns `true` if the first argument should come before
   *         the second.
   * @param array Pointer to the first element.
   * @param count Number of elements.
   * @param compare The comparison function.
   */
  template <typename T, typename Compare>
  void InsertionSort(T* array, Size count, Compare compare) {
    for (Size index = 1; index < count; ++index) {
      T current = array[index];
      Size position = index;

      while (position > 0 && compare(current, array[position - 1])) {
        array[position] = array[position - 1];

        --position;
      }

      array[position] = current;
    }
  }
}
