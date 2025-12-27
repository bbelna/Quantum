/**
 * @file System/Kernel/Include/Arch/IA32/Memory.hpp
 * @brief IA32 memory and paging management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 paging and memory functions.
   */
  class Memory {
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
       * Initializes paging with identity mappings based on the boot memory map.
       * @param bootInfoPhysicalAddress
       *   Physical address of boot info provided by bootloader.
       */
      static void InitializePaging(UInt32 bootInfoPhysicalAddress);

      /**
       * Allocates a 4 KB physical page (identity mapped).
       * @param zero
       *   Whether to zero the page before returning it.
       * @return
       *   Pointer to the allocated page.
       */
      static void* AllocatePage(bool zero = false);

      /**
       * Allocates a 4 KB physical page below a maximum address.
       * @param maxPhysicalAddress
       *   Maximum physical address (exclusive).
       * @param zero
       *   Whether to zero the page before returning it.
       * @param boundaryBytes
       *   Boundary in bytes that the returned page must not cross.
       * @return
       *   Pointer to the allocated page, or nullptr on failure.
       */
      static void* AllocatePageBelow(
        UInt32 maxPhysicalAddress,
        bool zero = false,
        UInt32 boundaryBytes = 0
      );

      /**
       * Frees a 4 KB physical page previously allocated.
       * @param page
       *   Pointer to the physical page to free.
       */
      static void FreePage(void* page);

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
       * Returns the physical address of the kernel page directory.
       * @return
       *   Physical address of the kernel page directory.
       */
      static UInt32 GetKernelPageDirectoryPhysical();

      /**
       * Creates a new address space and returns its page directory physical.
       * @return
       *   Physical address of the new page directory.
       */
      static UInt32 CreateAddressSpace();

      /**
       * Destroys an address space created by CreateAddressSpace.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to destroy.
       */
      static void DestroyAddressSpace(UInt32 pageDirectoryPhysical);

      /**
       * Maps a virtual page in the specified address space.
       * @param pageDirectoryPhysical
       *   Physical address of the target page directory.
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
      static void MapPageInAddressSpace(
        UInt32 pageDirectoryPhysical,
        UInt32 virtualAddress,
        UInt32 physicalAddress,
        bool writable = true,
        bool user = false,
        bool global = false
      );

      /**
       * Activates the specified address space.
       * @param pageDirectoryPhysical
       *   Physical address of the page directory to activate.
       */
      static void ActivateAddressSpace(UInt32 pageDirectoryPhysical);

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
       * Unmaps a virtual page.
       * Physical pages must be freed separately if desired.
       * @param virtualAddress
       *   Virtual address of the page to unmap.
       */
      static void UnmapPage(UInt32 virtualAddress);

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
       * Marks a physical range as used so it will not be handed out by the
       * allocator.
       * @param physicalAddress
       *   Base physical address of the range.
       * @param lengthBytes
       *   Length of the range in bytes.
       */
      static void ReservePhysicalRange(
        UInt32 physicalAddress,
        UInt32 lengthBytes
      );

      /**
       * Releases a previously reserved physical range back to the allocator.
       * @param physicalAddress
       *   Base physical address of the range.
       * @param lengthBytes
       *   Length of the range in bytes.
       */
      static void ReleasePhysicalRange(
        UInt32 physicalAddress,
        UInt32 lengthBytes
      );

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

      /**
       * Returns the total number of pages managed by the physical allocator.
       * @return
       *   Total number of pages.
       */
      static UInt32 GetPhysicalAllocatorTotalPages();

      /**
       * Returns the number of pages currently marked used by the allocator.
       * @return
       *   Number of used pages.
       */
      static UInt32 GetPhysicalAllocatorUsedPages();

      /**
       * Returns the number of pages currently free in the allocator.
       * @return
       *   Number of free pages.
       */
      static UInt32 GetPhysicalAllocatorFreePages();

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
       * Page present bit.
       */
      static constexpr UInt32 _pagePresent = 0x1;

      /**
       * Page writable bit.
       */
      static constexpr UInt32 _pageWrite = 0x2;

      /**
       * Page user-accessible bit.
       */
      static constexpr UInt32 _pageUser = 0x4;

      /**
       * Page global bit.
       */
      static constexpr UInt32 _pageGlobal = 0x100;

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
       * Maximum `BootInfo` entries to consume from firmware.
       */
      static constexpr UInt32 _maxBootEntries = 32;

      /**
       * Default managed memory size when no `BootInfo` is present.
       */
      static constexpr UInt32 _defaultManagedBytes = 64 * 1024 * 1024;

      /**
       * Total bytes under management by the physical allocator.
       */
      inline static UInt32 _managedBytes = _defaultManagedBytes;

      /**
       * Total number of 4 KB pages under management.
       */
      inline static UInt32 _pageCount = _defaultManagedBytes / _pageSize;

      /**
       * Number of pages currently marked used.
       */
      inline static UInt32 _usedPages = 0;

      /**
       * Kernel page directory storage.
       */
      alignas(_pageSize) static UInt32 _pageDirectory[_pageDirectoryEntries];

      /**
       * First page table used for initial identity mapping.
       */
      alignas(_pageSize) static UInt32 _firstPageTable[_pageTableEntries];

      /**
       * Bitmap tracking physical page usage.
       */
      inline static UInt32* _pageBitmap = nullptr;

      /**
       * Length of the page bitmap in 32-bit words.
       */
      inline static UInt32 _bitmapLengthWords = 0;

      /**
       * Start page index of the initial boot bundle.
       */
      inline static UInt32 _initBundleStartPage = 0;

      /**
       * End page index of the initial boot bundle.
       */
      inline static UInt32 _initBundleEndPage = 0;

      /**
       * Whether we've logged skipping INIT.BND pages yet.
       */
      inline static bool _loggedBundleSkip = false;

      /**
       * Allocates a single physical 4 KB page.
       * @param zero
       *   Whether to zero the page before returning it.
       * @return
       *   Physical address of the allocated page.
       */
      static UInt32 AllocatePhysicalPage(bool zero);

      /**
       * Converts a kernel virtual address into its physical load address.
       * @param virtualAddress
       *   Higher-half virtual address.
       * @return
       *   Physical address corresponding to the loaded image.
       */
      static UInt32 KernelVirtualToPhysical(UInt32 virtualAddress);

      /**
       * Computes a bit mask for a specific bit index.
       * @param bit
       *   Bit index within the bitmap.
       * @return
       *   Mask with the indexed bit set.
       */
      static UInt32 BitMask(UInt32 bit);

      /**
       * Converts a bit index to a bitmap word index.
       * @param bit
       *   Bit index within the bitmap.
       * @return
       *   Zero-based index of the 32-bit word containing the bit.
       */
      static UInt32 BitmapWordIndex(UInt32 bit);

      /**
       * Marks a page as used in the allocation bitmap.
       * @param pageIndex
       *   Page index to mark used.
       */
      static void SetPageUsed(UInt32 pageIndex);

      /**
       * Marks a page as free in the allocation bitmap.
       * @param pageIndex
       *   Page index to mark free.
       */
      static void ClearPageUsed(UInt32 pageIndex);

      /**
       * Tests whether a page is free according to the bitmap.
       * @param pageIndex
       *   Page index to query.
       * @return
       *   True if the page is free; false otherwise.
       */
      static bool PageFree(UInt32 pageIndex);

      /**
       * Tests whether a page is marked used according to the bitmap.
       * @param pageIndex
       *   Page index to query.
       * @return
       *   True if the page is used; false otherwise.
       */
      static bool PageUsed(UInt32 pageIndex);

      /**
       * Finds the index of the first zero bit; returns -1 if none.
       * @param value
       *   32-bit word to scan.
       * @return
       *   Bit index [0, 31] or -1 if all bits are one.
       */
      static int FindFirstZeroBit(UInt32 value);

      /**
       * Returns a pointer to the page directory entry array via the recursive
       * mapping.
       * @return
       *   Pointer to the page-directory entries.
       */
      static UInt32* GetPageDirectoryVirtual();

      /**
       * Returns a pointer to a page table entries array via the recursive
       * mapping.
       * @param pageDirectoryIndex
       *   Index of the page directory entry to project.
       * @return
       *   Pointer to the page-table entries for that page directory entry.
       */
      static UInt32* GetPageTableVirtual(UInt32 pageDirectoryIndex);

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
       * Initializes the physical page allocator using the provided boot info
       * map.
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot info block.
       */
      static void InitializePhysicalAllocator(UInt32 bootInfoPhysicalAddress);
  };
}
