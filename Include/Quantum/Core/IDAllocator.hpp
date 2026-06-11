/**
 * @file Include/Quantum/Core/IDAllocator.hpp
 * @brief Declares and implements @ref @QCore::IDAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Fixed-capacity ID allocator with \f$\mathcal{O}(1)\f$ allocate and
   *        free.
   * @tparam IDType The integer type used for IDs (e.g., @ref UInt32).
   * @tparam Capacity Maximum number of simultaneously live IDs. Also
   *         determines the size of the internal free stack.
   *
   * Maintains a monotonically increasing counter for fresh IDs and a stack
   * of freed IDs for reuse. Freed IDs are returned before new ones are
   * minted.
   */
  template <typename IDType, Size Capacity>
  class IDAllocator {
    public:
      /**
       * @brief Creates a new @ref IDAllocator instance that hands out IDs
       * starting at @p startID.
       * @param startID The first ID value to allocate.
       */
      explicit IDAllocator(IDType startID = 0)
        : _next(startID), _startID(startID), _freeCount(0) {}

      /**
       * @brief Allocates the next available ID.
       * @param out Receives the allocated ID on success.
       * @return `true` if an ID was allocated; `false` if the allocator is
       *         exhausted.
       */
      bool Allocate(IDType& out) {
        if (_freeCount > 0) {
          out = _freeStack[--_freeCount];

          return true;
        }

        if (_next - _startID >= static_cast<IDType>(Capacity))
          return false;

        out = _next++;

        return true;
      }

      /**
       * @brief Returns an ID to the free pool for later reuse.
       * @param id The ID to free. Must have been previously allocated.
       */
      void Free(IDType id) {
        if (_freeCount < Capacity)
          _freeStack[_freeCount++] = id;
      }

    private:
      /**
       * @brief The next ID to allocate if there are no freed IDs available.
       */
      IDType _next;

      /**
       * @brief The starting ID value (used to check capacity).
       */
      IDType _startID;

      /**
       * @brief The number of IDs currently in the free stack.
       */
      Size _freeCount;

      /**
       * @brief Stack of freed IDs available for reuse.
       */
      IDType _freeStack[Capacity];
  };
}
