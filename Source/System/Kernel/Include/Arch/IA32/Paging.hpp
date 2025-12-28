/**
 * @file System/Kernel/Include/Arch/IA32/Paging.hpp
 * @brief IA32 paging support.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 paging helpers.
   */
  class Paging {
    public:
      /**
       * Base virtual address where the kernel will be mapped in the
       * higher-half. Identity mappings remain available for now to ease the
       * transition.
       */
      static constexpr UInt32 kernelVirtualBase = 0xC0000000;

      /**
       * Page-directory slot reserved for the recursive/self map.
       */
      static constexpr UInt32 recursiveSlot = 1023;

      /**
       * Size of a single page in bytes.
       */
      static constexpr UInt32 pageSize = 4096;

      /**
       * Base virtual address for the kernel heap region.
       */
      static constexpr UInt32 kernelHeapBase = 0xC2000000;

      /**
       * Total virtual bytes reserved for the kernel heap region.
       */
      static constexpr UInt32 kernelHeapBytes = 512 * 1024 * 1024;

      /**
       * Page present bit.
       */
      static constexpr UInt32 pagePresent = 0x1;

      /**
       * Page writable bit.
       */
      static constexpr UInt32 pageWrite = 0x2;

      /**
       * Page user-accessible bit.
       */
      static constexpr UInt32 pageUser = 0x4;

      /**
       * Page global bit.
       */
      static constexpr UInt32 pageGlobal = 0x100;

      /**
       * Initializes paging with identity mappings based on the boot memory map.
       * @param bootInfoPhysicalAddress
       *   Physical address of boot info provided by bootloader.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Maps a virtual page to a physical page with present/RW bits set.
       * Assumes identity mapping for page tables themselves.
       * @param virtualAddress
       *   Virtual address of the page to map.
       * @param physicalAddress
       *   Physical address of the page to map.
       * @param writable
       *   Whether the page should be writable.
       * @param user
       *   Whether the page should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapPage(
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Unmaps a virtual page.
       * Physical pages must be freed separately if desired.
       * @param virtualAddress
       *   Virtual address of the page to unmap.
       */
      static void UnmapPage(UInt32 virtualAddress);

      /**
       * Maps a contiguous virtual range to a contiguous physical range.
       * @param virtualAddress
       *   Base virtual address (page aligned).
       * @param physicalAddress
       *   Base physical address (page aligned).
       * @param lengthBytes
       *   Length of range in bytes.
       * @param writable
       *   Whether the pages should be writable.
       * @param user
       *   Whether the pages should be user accessible.
       * @param global
       *   Whether the mapping should be marked global.
       */
      static void MapRange(
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        UInt32 lengthBytes,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Unmaps a contiguous virtual range.
       * Physical pages must be freed separately if desired.
       * @param virtualAddress
       *   Base virtual address (page aligned).
       * @param lengthBytes
       *   Length of range in bytes.
       */
      static void UnmapRange(UInt32 virtualAddress, UInt32 lengthBytes);

      /**
       * Returns the page directory entry value for a given virtual address.
       * @param virtualAddress
       *   Virtual address to query.
       * @return
       *  Page directory entry value for the given virtual address.
       */
      static UInt32 GetPageDirectoryEntry(UInt32 virtualAddress);

      /**
       * Returns the page table entry value for a given virtual address.
       * @param virtualAddress
       *   Virtual address to query.
       * @return
       *   Page table entry value for the given virtual address.
       */
      static UInt32 GetPageTableEntry(UInt32 virtualAddress);

      /**
       * Returns the physical address of the kernel page directory.
       * @return
       *   Physical address of the kernel page directory.
       */
      static UInt32 GetKernelPageDirectoryPhysicalAddress();

      /**
       * Returns the physical address of the first page table.
       * @return
       *   Physical address of the first page table.
       */
      static UInt32 GetFirstPageTablePhysicalAddress();

      /**
       * Returns the kernel page directory entries.
       * @return
       *   Pointer to the kernel page directory entries.
       */
      static const UInt32* GetKernelPageDirectoryEntries();

      /**
       * Handles a page fault.
       * @param context
       *   Interrupt context at the time of the fault.
       * @param faultAddress
       *   Address that triggered the fault (CR2).
       * @param errorCode
       *   Page-fault error code from the CPU.
       * @return
       *   True if the fault was handled; false if it should escalate.
       */
      static bool HandlePageFault(
        const Interrupts::Context& context,
        UInt32 faultAddress,
        UInt32 errorCode
      );

    private:
      /**
       * Size of a single page in bytes.
       */
      static constexpr UInt32 _pageSize = pageSize;

      /**
       * Number of entries per IA32 page directory.
       */
      static constexpr UInt32 _pageDirectoryEntries = 1024;

      /**
       * Number of entries per IA32 page table.
       */
      static constexpr UInt32 _pageTableEntries = 1024;

      /**
       * Virtual address base exposing all page table entries via the recursive
       * slot.
       */
      static constexpr UInt32 _recursivePageTablesBase = 0xFFC00000;

      /**
       * Virtual address exposing the page directory entries array via the
       * recursive slot.
       */
      static constexpr UInt32 _recursivePageDirectory = 0xFFFFF000;

      /**
       * Kernel page directory storage.
       */
      alignas(_pageSize) static UInt32 _pageDirectory[_pageDirectoryEntries];

      /**
       * First page table used for initial identity mapping.
       */
      alignas(_pageSize) static UInt32 _firstPageTable[_pageTableEntries];

      /**
       * Returns a pointer to the page directory entry array via the recursive
       * mapping.
       * @return
       *   Pointer to the page-directory entries.
       */
      static UInt32* GetPageDirectoryVirtualAddress();

      /**
       * Returns a pointer to a page table entries array via the recursive
       * mapping.
       * @param pageDirectoryIndex
       *   Index of the page directory entry to project.
       * @return
       *   Pointer to the page-table entries for that page directory entry.
       */
      static UInt32* GetPageTableVirtualAddress(UInt32 pageDirectoryIndex);

      /**
       * Ensures a page table exists for a page directory entry index,
       * allocating if needed.
       * @param pageDirectoryIndex
       *   Index of the page directory entry to populate.
       * @return
       *   Pointer to the mapped page-table entries.
       */
      static UInt32* EnsurePageTable(UInt32 pageDirectoryIndex);

      /**
       * Ensures page tables exist for the kernel heap region so all address
       * spaces can share them.
       */
      static void EnsureKernelHeapTables();

      /**
       * Tracks whether paging is active (recursive mapping usable).
       */
      inline static bool _pagingActive = false;
  };
}
