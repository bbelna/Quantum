/**
 * @file Kernel/Memory/IMemoryMapper.hpp
 * @brief Declares @ref @QKrnl::Memory::IMemoryMapper.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "IAddressSpace.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Abstract interface for mapping @ref MemoryBlock instances in
   *        an @ref IAddressSpace into the kernel's @ref IAddressSpace.
   *
   * Architecture-specific implementations (e.g., the IA-32 page-table mapper)
   * inherit from this interface.
   */
  class IMemoryMapper {
    public:
      /**
       * @brief Maps a @ref MemoryBlock in an @ref IAddressSpace to the kernel's
       *        @ref IAddressSpace with the given @p flags.
       * @param addressSpace The @ref IAddressSpace to map from.
       * @param block The @ref MemoryBlock to map.
       * @param flags The @ref MemoryMappingFlags to map with.
       * @return The mapped @ref MemoryBlock in the kernel's @ref IAddressSpace,
       *         or an invalid @ref MemoryBlock on failure.
       */
      virtual MemoryBlock Map(
        IAddressSpace& addressSpace,
        MemoryBlock block,
        const MemoryMappingFlags& flags
      ) = 0;

      /**
       * @brief Maps a @ref MemoryBlock in an @ref IAddressSpace to a specific
       *        @ref MemoryBlock in the kernel's @ref IAddressSpace.
       * @param addressSpace The @ref IAddressSpace to map from.
       * @param fromBlock The @ref MemoryBlock to map.
       * @param toBlock The @ref MemoryBlock to map to.
       * @param flags The @ref MemoryMappingFlags to map with.
       * @return The mapped @ref MemoryBlock in the kernel's @ref IAddressSpace,
       *         or an invalid @ref MemoryBlock on failure.
       */
      virtual MemoryBlock Map(
        IAddressSpace& addressSpace,
        MemoryBlock fromBlock,
        MemoryBlock toBlock,
        const MemoryMappingFlags& flags
      ) = 0;

      /**
       * @brief Unmaps a @ref MemoryBlock in an @ref IAddressSpace, returning
       *        the backing @ref MemoryBlock in the kernel's @ref IAddressSpace.
       * @param addressSpace The @ref IAddressSpace to unmap from.
       * @param block The @ref MemoryBlock to unmap.
       * @return The backing @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      virtual MemoryBlock Unmap(
        IAddressSpace& addressSpace,
        MemoryBlock block
      ) = 0;

      /**
       * @brief Resolves a @ref MemoryBlock to its backing kernel
       *        @ref MemoryBlock without modifying the mapping.
       * @param addressSpace The @ref IAddressSpace to resolve in.
       * @param block The @ref MemoryBlock to resolve.
       * @return The backing kernel @ref MemoryBlock, or an invalid
       *         @ref MemoryBlock if not mapped.
       */
      virtual MemoryBlock Resolve(
        IAddressSpace& addressSpace,
        MemoryBlock block
      ) = 0;
  };
}
