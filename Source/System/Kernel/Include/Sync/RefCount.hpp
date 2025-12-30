/**
 * @file System/Kernel/Include/Sync/RefCount.hpp
 * @brief Atomic reference counting helper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Atomics.hpp"

namespace Quantum::System::Kernel::Sync {
  /**
   * Simple atomic reference counter.
   */
  class RefCount {
    public:
      /**
       * Initializes the reference count.
       * @param value
       *   Initial reference count value (default: 1).
       */
      void Initialize(UInt32 value = 1) {
        _count.Store(value);
      }

      /**
       * Increments the reference count.
       * @return
       *   New reference count value.
       */
      UInt32 AddRef() {
        return _count.FetchAdd(1) + 1;
      }

      /**
       * Decrements the reference count.
       * @return
       *   New reference count value.
       */
      UInt32 Release() {
        return _count.FetchAdd(static_cast<UInt32>(-1)) - 1;
      }

      /**
       * Reads the current reference count.
       * @return
       *   Current reference count value.
       */
      UInt32 Get() const {
        return _count.Load();
      }

    private:
      /**
       * Reference count value.
       */
      Atomic<UInt32> _count;
  };
}
