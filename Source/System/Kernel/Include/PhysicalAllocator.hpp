/**
 * @file System/Kernel/Include/PhysicalAllocator.hpp
 * @brief Physical page allocator wrapper.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * Physical page allocator wrapper.
   */
  class PhysicalAllocator {
    public:
      /**
       * Physical page size in bytes.
       */
      static constexpr UInt32 pageSize = 4096;

      /**
       * Allocates one physical page.
       * @param zero
       *   Whether to zero the page contents.
       * @return
       *   Pointer to the allocated page (identity mapped).
       */
      static void* AllocatePage(bool zero = false);

      /**
       * Frees a physical page (identity-mapped).
       * @param page
       *   Pointer to the page to free.
       */
      static void FreePage(void* page);

      /**
       * Returns total physical pages tracked by the allocator.
       * @return
       *   Total number of pages.
       */
      static UInt32 GetTotalPages();

      /**
       * Returns used physical pages tracked by the allocator.
       * @return
       *   Used number of pages.
       */
      static UInt32 GetUsedPages();

      /**
       * Returns free physical pages tracked by the allocator.
       * @return
       *   Free number of pages.
       */
      static UInt32 GetFreePages();
  };
}
