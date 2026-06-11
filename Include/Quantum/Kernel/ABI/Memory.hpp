/**
 * @file Include/Quantum/Kernel/ABI/Memory.hpp
 * @brief Declaration of the memory ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "../Arch.hpp"
#include "../Memory.hpp"
#include "../Resources.hpp"
#include "ABI.hpp"

/**
 * @brief ABI functions for memory management.
 */
namespace Quantum::Kernel::Memory::ABI {
  /**
   * @brief Information about the system's physical memory usage.
   */
  struct MemoryInfo {
    /**
     * @brief Total physical memory under management, in bytes.
     */
    Size TotalBytes;

    /**
     * @brief Used physical memory, in bytes.
     */
    Size UsedBytes;

    /**
     * @brief Free physical memory, in bytes.
     */
    Size FreeBytes;

    /**
     * @brief Size of a single physical memory block, in bytes.
     */
    Size BlockSize;

    /**
     * @brief Total number of physical memory blocks.
     */
    Size BlockCount;

    /**
     * @brief Number of used physical memory blocks.
     */
    Size UsedBlockCount;

    /**
     * @brief Physical memory occupied by the initial image (startup bundle),
     *        in bytes. Drops to 0 after the startup server frees it.
     */
    Size InitialImageBytes;

    /**
     * @brief Physical memory permanently reserved at boot (kernel binary,
     *        allocator bitmap, ref-count array, tag array, boot stack),
     *        in bytes.
     */
    Size KernelReservedBytes;

    /**
     * @brief Physical memory mapped for the kernel heap, in bytes.
     */
    Size KernelHeapBytes;

    /**
     * @brief Physical memory attributed to user processes (sum of all
     *        processes' TotalPageCount), in bytes.
     */
    Size UserProcessBytes;
  };

  /**
   * @brief Memory pressure levels visible to user-space.
   */
  enum class PressureState : UInt8 {
    Normal = 0,
    Elevated = 1,
    Critical = 2
  };

  /**
   * @brief Memory pressure diagnostics snapshot.
   */
  struct MemoryPressureInfo {
    Size TotalBlocks;
    Size FreeBlocks;
    Size UsedBlocks;
    Size KernelHeapBytes;
    Size SharedBufferCount;
    Size PoolIPCMessageBytes;
    Size PoolIPCPortBytes;
    Size PoolSharedBufferBytes;
    PressureState State;
  };

  /**
   * @brief Gets memory pressure diagnostics.
   * @param info Pointer to a `MemoryPressureInfo` structure to fill.
   * @return `true` on success; `false` on failure.
   */
  inline bool GetPressureInfo(MemoryPressureInfo* info) {
    return ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_GetPressureInfo,
      reinterpret_cast<UInt32>(info)
    ) != 0;
  }

  /**
   * @brief Maximum number of kernel block tags.
   */
  constexpr Size MaxBlockTags = 12;

  /**
   * @brief Per-tag kernel memory usage snapshot.
   *
   * Each entry in `BlockCounts` corresponds to a `MemoryBlockTag` value
   * (0 = Unknown, 1 = PageTable, 2 = ProcessImage, ... 11 = Reserved).
   */
  struct MemoryTagStats {
    /**
     * @brief Block size in bytes (same as MemoryInfo::BlockSize).
     */
    Size BlockSize;

    /**
     * @brief Number of allocated blocks for each tag index.
     */
    Size BlockCounts[MaxBlockTags];

    /**
     * @brief Human-readable label for each tag index (null-terminated).
     */
    char TagNames[MaxBlockTags][20];
  };

  /**
   * @brief Gets per-tag kernel memory block counts.
   * @param stats Pointer to a `MemoryTagStats` structure to fill.
   * @return `true` on success; `false` on failure.
   */
  inline bool GetTagStats(MemoryTagStats* stats) {
    return ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_GetTagStats,
      reinterpret_cast<UInt32>(stats)
    ) != 0;
  }

  /**
   * @brief Gets information about the system's physical memory usage.
   * @param info Pointer to a `MemoryInfo` structure to fill.
   * @return `true` on success; `false` on failure.
   */
  inline bool GetInfo(MemoryInfo* info) {
    return ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_GetInfo,
      reinterpret_cast<UInt32>(info)
    ) != 0;
  }

  /**
   * @brief Allocates a block of memory in the calling process' address space.
   * @param sizeInBytes The size of the block to allocate, in bytes.
   * @return The virtual address of the allocated block, or null on failure.
   */
  inline UIntPtr Allocate(Size sizeInBytes) {
    UInt32 result = ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_Allocate,
      sizeInBytes
    );

    return static_cast<UIntPtr>(result);
  }

  /**
   * @brief Frees a previously allocated block of memory in the calling
   *        process' address space.
   * @param address The virtual address of the block to free.
   */
  inline void Free(UIntPtr address) {
    ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_Free,
      address
    );
  }

  /**
   * @brief Reserves a contiguous virtual address range in the calling
   *        process' address space. Pages are lazily allocated on first
   *        access via demand paging.
   * @param sizeInBytes The size of the range to reserve, in bytes.
   * @return The virtual address of the reserved range, or null on failure.
   */
  inline UIntPtr AllocateRange(Size sizeInBytes) {
    UInt32 result = ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_AllocateRange,
      sizeInBytes
    );

    return static_cast<UIntPtr>(result);
  }

  /**
   * @brief Creates a shared memory buffer backed by reference-counted
   *        physical pages and maps it into the caller's address space.
   * @param sizeInBytes The size of the buffer in bytes.
   * @return The SharedBufferID on success, or 0 on failure.
   */
  inline Kernel::Memory::SharedBufferID CreateShared(Size sizeInBytes) {
    return static_cast<Kernel::Memory::SharedBufferID>(
      ::Quantum::Kernel::ABI::Invoke(
        KernelOperation::Memory_CreateShared,
        sizeInBytes
      )
    );
  }

  /**
   * @brief Maps an existing shared buffer into the caller's address space.
   * @param id The SharedBufferID returned by `CreateShared`.
   * @return The virtual address of the mapping, or null on failure.
   */
  inline UIntPtr AttachShared(Kernel::Memory::SharedBufferID id) {
    UInt32 result = ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_AttachShared,
      static_cast<UInt32>(id)
    );

    return static_cast<UIntPtr>(result);
  }

  /**
   * @brief Unmaps a shared buffer from the caller's address space.
   * @param address The virtual address of the attached mapping.
   */
  inline void DetachShared(UIntPtr address) {
    ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_DetachShared,
      address
    );
  }

  /**
   * @brief Allocates memory whose physical address is below a given limit
   *        and maps it into the caller's address space. Used by drivers
   *        that program DMA controllers with restricted address ranges.
   * @param sizeInBytes The size of the block to allocate, in bytes.
   * @param physicalLimit The exclusive upper bound on the physical address.
   * @return The virtual address of the allocated block, or null on failure.
   */
  inline UIntPtr AllocateDMA(Size sizeInBytes, UInt32 physicalLimit) {
    UInt32 result = ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_AllocateDMA,
      sizeInBytes,
      physicalLimit
    );

    return static_cast<UIntPtr>(result);
  }

  /**
   * @brief Translates a virtual address to its underlying physical address
   *        in the calling process' page tables. Primarily used by DMA
   *        drivers that must program a hardware DMA controller with a
   *        physical address.
   * @param address The virtual address to translate.
   * @return The physical address, or 0 if the address is unmapped or
   *         otherwise invalid.
   */
  inline UInt32 VirtualToPhysical(UIntPtr address) {
    return ::Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_VirtualToPhysical,
      address
    );
  }
}
