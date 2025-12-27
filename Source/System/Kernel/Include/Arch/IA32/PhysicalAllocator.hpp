/**
 * @file System/Kernel/Include/Arch/IA32/PhysicalAllocator.hpp
 * @brief IA32 physical page allocator.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 physical page allocator.
   */
  class PhysicalAllocator {
    public:
      /**
       * Size of a single page in bytes.
       */
      static constexpr UInt32 pageSize = 4096;

      /**
       * Initializes the physical allocator from the boot info map.
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot info block.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Allocates a single physical 4 KB page.
       * @param zero
       *   Whether to zero the page before returning it.
       * @return
       *   Physical address of the allocated page.
       */
      static UInt32 AllocatePage(bool zero);

      /**
       * Allocates a physical 4 KB page below a maximum address.
       * @param maxPhysicalAddress
       *   Maximum physical address (exclusive).
       * @param zero
       *   Whether to zero the page before returning it.
       * @param boundaryBytes
       *   Boundary in bytes that the returned page must not cross.
       * @return
       *   Physical address of the allocated page, or 0 on failure.
       */
      static UInt32 AllocatePageBelow(
        UInt32 maxPhysicalAddress,
        bool zero,
        UInt32 boundaryBytes
      );

      /**
       * Frees a physical 4 KB page previously allocated.
       * @param physicalAddress
       *   Physical address of the page to free.
       */
      static void FreePage(UInt32 physicalAddress);

      /**
       * Marks a physical range as used so it will not be handed out.
       * @param physicalAddress
       *   Base physical address of the range.
       * @param lengthBytes
       *   Length of the range in bytes.
       */
      static void ReserveRange(UInt32 physicalAddress, UInt32 lengthBytes);

      /**
       * Releases a previously reserved physical range.
       * @param physicalAddress
       *   Base physical address of the range.
       * @param lengthBytes
       *   Length of the range in bytes.
       */
      static void ReleaseRange(UInt32 physicalAddress, UInt32 lengthBytes);

      /**
       * Returns the total number of pages managed by the allocator.
       * @return
       *   Total number of pages.
       */
      static UInt32 GetTotalPages();

      /**
       * Returns the number of pages currently marked used.
       * @return
       *   Used number of pages.
       */
      static UInt32 GetUsedPages();

      /**
       * Returns the number of free pages.
       * @return
       *   Free number of pages.
       */
      static UInt32 GetFreePages();

      /**
       * Returns total managed bytes.
       * @return
       *   Managed bytes.
       */
      static UInt32 GetManagedBytes();

      /**
       * Converts a kernel virtual address into its physical load address.
       * @param virtualAddress
       *   Higher-half virtual address.
       * @return
       *   Physical address corresponding to the loaded image.
       */
      static UInt32 KernelVirtualToPhysical(UInt32 virtualAddress);

    private:
      /**
       * Maximum `BootInfo` entries to consume from firmware.
       */
      static constexpr UInt32 _maxBootEntries = 32;

      /**
       * Default managed memory size when no `BootInfo` is present.
       */
      static constexpr UInt32 _defaultManagedBytes = 64 * 1024 * 1024;

      /**
       * Total bytes under management by the allocator.
       */
      inline static UInt32 _managedBytes = _defaultManagedBytes;

      /**
       * Total number of 4 KB pages under management.
       */
      inline static UInt32 _pageCount = _defaultManagedBytes / pageSize;

      /**
       * Number of pages currently marked used.
       */
      inline static UInt32 _usedPages = 0;

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
  };
}
