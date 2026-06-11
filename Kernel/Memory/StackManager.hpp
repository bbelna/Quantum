/**
 * @file Kernel/Memory/StackManager.hpp
 * @brief Declares @ref @QKrnl::Memory::StackManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "AddressSpaceMap.hpp"
#include "HeapAllocator.hpp"
#include "Stack.hpp"
#include "StackManagerConfiguration.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Manages @ref Stack across @ref IAddressSpace instances.
   */
  class StackManager {
    public:
      /**
       * @brief Creates a new @ref StackManager.
       * @param memoryAllocator
       *   Reference to a concrete implementation of @ref IMemoryAllocator.
       * @param memoryMapper
       *   Reference to a concrete implementation of @ref IMemoryMapper.
       * @param configuration
       *   @ref StackManagerConfiguration parameters.
       */
      explicit StackManager(
        IMemoryAllocator& memoryAllocator,
        IMemoryMapper& memoryMapper,
        StackManagerConfiguration configuration
      );

      /**
       * @brief
       *   Creates a @ref Stack in the specified @ref IAddressSpace with
       *   the given @ref MemoryMappingPermissions.
       * @param addressSpace
       *   Reference to the @ref IAddressSpace to create the @ref Stack in.
       * @param sizeInBytes
       *   The size of the @ref Stack to create in bytes.
       * @param permissions
       *   The @ref MemoryMappingPermissions to use for the @ref Stack
       *   @ref MemoryBlock instances.
       * @return
       *   Pointer to the new @ref Stack in the @ref IAddressSpace, or a
       *   @ref Stack with `null` base on failure.
       */
      Stack* Create(
        IAddressSpace& addressSpace,
        Size sizeInBytes,
        MemoryMappingPermissions permissions = MemoryMappingPermissions::None
      );

      /**
       * @brief Deletes a @ref Stack in the specified @ref IAddressSpace.
       * @param addressSpace Reference to the @ref IAddressSpace the
       *                     @ref Stack is in.
       * @param stack Pointer to the @ref Stack to delete.
       */
      void Delete(IAddressSpace& addressSpace, Stack* stack);

    private:
      /**
       * @brief Reference to a concrete implementation of
       *        @ref IMemoryAllocator.
       */
      IMemoryAllocator& _memoryAllocator;

      /**
       * @brief Reference to a concrete implementation of @ref IMemoryMapper.
       */
      IMemoryMapper& _virtualBlockMapper;

      /**
       * @brief @ref MemoryBlock representing the entire stack region from
       *        which individual stacks will be allocated.
       */
      MemoryBlock _stackBlock;

      /**
       * @brief Number of guard @ref MemoryBlock per @ref Stack.
       */
      Size _guardBlockCount;

      /**
       * @brief @ref Spinlock for synchronizing access.
       */
      Spinlock<UInt32>* _spinlock = nullptr;

      /**
       * @brief Free list of available stacks.
       */
      LinkedNode<MemoryBlock>* _freeList = nullptr;

      /**
       * @brief Maps a @ref Stack into the specified @ref IAddressSpace.
       * @param addressSpace Reference to the @ref IAddressSpace.
       * @param stackBlock The @ref Stack to map.
       * @return `true` on success; `false` on failure.
       */
      bool _map(
        IAddressSpace& addressSpace,
        Stack stack,
        MemoryMappingPermissions permissions
          = MemoryMappingPermissions::None
      );

      /**
       * @brief Unmaps the given @ref Stack from the specified
       *        @ref IAddressSpace.
       * @param addressSpace Reference to the @ref IAddressSpace.
       * @param stack The @ref Stack to unmap.
       * @return `true` on success; `false` on failure.
       */
      bool _unmap(
        IAddressSpace& addressSpace,
        Stack stack
      );

      /**
       * @brief Releases a @ref MemoryBlock slot to the free list.
       * @param slot The @ref MemoryBlock slot to release.
       */
      void _releaseSlot(MemoryBlock slot);
  };
}
