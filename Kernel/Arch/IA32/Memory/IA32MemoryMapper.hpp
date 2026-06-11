/**
 * @file Kernel/Arch/IA32/Memory/IA32MemoryMapper.hpp
 * @brief Declares @ref @QKrnlIA32::Memory::MemoryMapper.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>
#include <Memory/IMemoryMapper.hpp>

#include "IA32PageDirectoryManager.hpp"
#include "IA32PageTableFlags.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief IA-32 implementation of @ref @QKrnl::Memory::IMemoryMapper.
   *
   * Translates between process memory blocks by manipulating IA-32 page tables.
   * Uses the @ref IA32PageDirectoryManager to ensure page tables exist and the
   * @ref IA32MemoryAllocator to allocate backing frames.
   */
  class IA32MemoryMapper : public IMemoryMapper {
    public:
      /**
       * @brief
       *   Initializes the @ref IA32MemoryMapper.
       * @param cpu
       *   Pointer to the @ref IA32CPUDriver.
       * @param memoryAllocator
       *   Pointer to the @ref IMemoryAllocator.
       * @param pageDirectoryManager
       *   Pointer to the @ref IA32PageDirectoryManager.
       */
      void Initialize(
        IA32CPUDriver* cpu,
        IMemoryAllocator* memoryAllocator,
        IA32PageDirectoryManager* pageDirectoryManager
      );

      /**
       * @brief Maps a @ref MemoryBlock in the given @ref IAddressSpace.
       * @param addressSpace Reference to the @ref IAddressSpace to map in.
       * @param block The @ref MemoryBlock to map.
       * @param flags The @ref MemoryMappingFlags for the mapping.
       * @return The mapped @ref MemoryBlock, or an invalid @ref MemoryBlock on
       *         failure.
       */
      MemoryBlock Map(
        IAddressSpace& addressSpace,
        MemoryBlock block,
        const MemoryMappingFlags& flags
      ) override;

      /**
       * @brief Unmaps a @ref MemoryBlock and returns the backing
       *        @ref MemoryBlock.
       * @param addressSpace Reference to the @ref IAddressSpace to unmap from.
       * @param block The @ref MemoryBlock to unmap.
       * @return The backing @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      MemoryBlock Map(
        IAddressSpace& addressSpace,
        MemoryBlock virtualBlock,
        MemoryBlock kernelBlock,
       const MemoryMappingFlags& flags
      ) override;

      /**
       * @brief Unmaps a @ref MemoryBlock and returns the backing
       *        @ref MemoryBlock.
       * @param addressSpace Reference to the @ref IAddressSpace to unmap from.
       * @param block The @ref MemoryBlock to unmap.
       * @return The backing @ref MemoryBlock, or an invalid @ref MemoryBlock
       *         on failure.
       */
      MemoryBlock Unmap(
        IAddressSpace& addressSpace,
        MemoryBlock block
      ) override;

      /**
       * @brief Resolves a mapped @ref MemoryBlock.
       * @param addressSpace Reference to the @ref IAddressSpace to resolve in.
       * @param block The @ref MemoryBlock to resolve.
       * @return The backing @ref MemoryBlock, or an invalid @ref MemoryBlock if
       *         not mapped.
       */
      MemoryBlock Resolve(
        IAddressSpace& addressSpace,
        MemoryBlock block
      ) override;

    private:
      /**
       * @brief Pointer to the @ref IA32CPUDriver.
       */
      IA32CPUDriver* _cpu = nullptr;

      /**
       * @brief Pointer to the kernel @ref IMemoryAllocator.
       */
      IMemoryAllocator* _memoryAllocator = nullptr;

      /**
       * @brief Pointer to the @ref IA32PageDirectoryManager.
       */
      IA32PageDirectoryManager* _pageDirectoryManager = nullptr;
  };
}
