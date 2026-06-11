/**
 * @file Kernel/Memory/IAddressSpaceAllocator.hpp
 * @brief Declares @ref @QKrnl::Memory::IAddressSpaceAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IAddressSpace.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Abstract interface for allocating and freeing @ref IAddressSpace.
   *
   * Architecture-specific implementations (e.g., the IA-32 page-directory
   * allocator) inherit from this interface so that kernel subsystems can
   * create and destroy address spaces without depending on architecture
   * headers.
   */
  class IAddressSpaceAllocator {
    public:
      /**
       * @brief Allocates a new, empty @ref IAddressSpace.
       * @return Pointer to the newly allocated @ref IAddressSpace, or `nullptr`
       *         if allocation fails.
       *
       * The returned @ref IAddressSpace is ready to have memory mapped into it
       * via @ref IMemoryMapper. The caller takes ownership and must
       * eventually release it with @ref Free.
       */
      virtual IAddressSpace* Allocate() = 0;

      /**
       * @brief Frees a previously allocated @ref IAddressSpace.
       * @param addressSpace Pointer to the @ref IAddressSpace to free.
       *
       * The caller must ensure that all mappings within the @ref IAddressSpace
       * have been removed before calling this method. Passing `nullptr`
       * is undefined behavior.
       */
      virtual void Free(IAddressSpace* addressSpace) = 0;
  };
}
