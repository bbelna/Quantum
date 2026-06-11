/**
 * @file Kernel/Memory/MemoryBlockTag.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryBlockTag.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Categorizes why a @ref MemoryBlock was allocated.
   *
   * Stored per-block alongside the reference count so that memory audits
   * and the OOM diagnostic dump can report usage by category.
   */
  enum class MemoryBlockTag : UInt8 {
    /**
     * @brief Untagged allocation (legacy callers not yet updated).
     */
    Unknown = 0,

    /**
     * @brief Page directory or page table for process or kernel address
     *        space management.
     */
    PageTable = 1,

    /**
     * @brief Code, data, or BSS segments mapped into a process during
     *        spawn.
     */
    ProcessImage = 2,

    /**
     * @brief User-space heap pages allocated via `Memory_Allocate` or
     *        `Memory_AllocateRange` syscalls.
     */
    ProcessHeap = 3,

    /**
     * @brief Kernel or user thread stack pages.
     */
    Stack = 4,

    /**
     * @brief Shared memory buffer backing pages allocated via
     *        `Memory_CreateShared`.
     */
    SharedBuffer = 5,

    /**
     * @brief Pages mapped to hold received IPC messages in user space
     *        (`IPC_Receive`, `IPC_TryReceive`, `IPC_ReceiveAny`).
     */
    IPCReceiveBuffer = 6,

    /**
     * @brief Kernel heap slab pages grown by the @ref HeapAllocator.
     */
    HeapSlab = 7,

    /**
     * @brief Pages allocated for demand-paging (page fault handler
     *        lazy mapping).
     */
    DemandPage = 8,

    /**
     * @brief DMA-safe pages allocated below a physical address limit.
     */
    DMA = 9,

    /**
     * @brief Pages allocated to map device information to user space
     *        (e.g. `Platform_GetDevices`).
     */
    DeviceInfo = 10,

    /**
     * @brief Reserved blocks (kernel image, bitmap, refcount array,
     *        boot stack, initial image).
     */
    Reserved = 11,

    /**
     * @brief Sentinel: number of tag values. Must be last.
     */
    Count = 12
  };

  /**
   * @brief Returns a human-readable label for a block tag.
   * @param tag The tag to label.
   * @return A short static string, e.g. `"PageTable"`, `"ProcessHeap"`.
   */
  inline const char* MemoryBlockTagName(MemoryBlockTag tag) {
    switch (tag) {
      case MemoryBlockTag::Unknown:          return "Unknown";
      case MemoryBlockTag::PageTable:        return "PageTable";
      case MemoryBlockTag::ProcessImage:     return "ProcessImage";
      case MemoryBlockTag::ProcessHeap:      return "ProcessHeap";
      case MemoryBlockTag::Stack:            return "Stack";
      case MemoryBlockTag::SharedBuffer:     return "SharedBuffer";
      case MemoryBlockTag::IPCReceiveBuffer: return "IPCReceiveBuffer";
      case MemoryBlockTag::HeapSlab:         return "HeapSlab";
      case MemoryBlockTag::DemandPage:       return "DemandPage";
      case MemoryBlockTag::DMA:              return "DMA";
      case MemoryBlockTag::DeviceInfo:       return "DeviceInfo";
      case MemoryBlockTag::Reserved:         return "Reserved";
      default:                               return "?";
    }
  }
}
