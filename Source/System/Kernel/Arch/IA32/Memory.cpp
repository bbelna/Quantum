//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Memory.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// IA32 paging setup and simple physical page allocator.
//------------------------------------------------------------------------------

#include <Arch/IA32/LinkerSymbols.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Memory.hpp>
#include <BootInfo.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using LogLevel = Logger::Level;

  namespace {
    constexpr UInt32 pageSize = 4096;
    constexpr UInt32 pagePresent = 0x1;
    constexpr UInt32 pageWrite = 0x2;
    constexpr UInt32 maxBootEntries = 32;
    constexpr UInt32 defaultManagedBytes = 64 * 1024 * 1024;

    UInt32 managedBytes = defaultManagedBytes;
    UInt32 pageCount = defaultManagedBytes / pageSize;

    static UInt32 pageDirectory[1024] __attribute__((aligned(pageSize)));
    static UInt32 firstPageTable[1024] __attribute__((aligned(pageSize)));
    static UInt32* pageBitmap = nullptr;
    static UInt32 bitmapLengthWords = 0;

    /**
     * Aligns a value up to the specified alignment boundary.
     * @param value Value to align.
     * @param alignment Alignment to apply (must be power of two).
     * @return Aligned value.
     */
    inline UInt32 AlignUp(UInt32 value, UInt32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }

    /**
     * Marks a page as used in the bitmap.
     * @param pageIndex Index of the page to mark.
     */
    inline void SetPageUsed(UInt32 pageIndex) {
      pageBitmap[pageIndex / 32] |= (1u << (pageIndex % 32));
    }

    /**
     * Clears the used bit for a page in the bitmap.
     * @param pageIndex Index of the page to clear.
     */
    inline void ClearPageUsed(UInt32 pageIndex) {
      pageBitmap[pageIndex / 32] &= ~(1u << (pageIndex % 32));
    }

    /**
     * Tests whether a page is free in the bitmap.
     * @param pageIndex Index of the page to test.
     * @return true if the page is free; false otherwise.
     */
    inline bool PageFree(UInt32 pageIndex) {
      return (pageBitmap[pageIndex / 32] & (1u << (pageIndex % 32))) == 0;
    }

    /**
     * Initializes the physical page allocator using the provided boot info map.
     * @param bootInfoPhysicalAddress Physical address of the boot info block.
     */
    void InitializePhysicalAllocator(UInt32 bootInfoPhysicalAddress) {
      BootInfo* bootInfo = nullptr;

      if (
        bootInfoPhysicalAddress >= pageSize &&
        bootInfoPhysicalAddress < managedBytes
      ) {
        bootInfo = reinterpret_cast<BootInfo*>(bootInfoPhysicalAddress);
      }

      // Track the highest usable address from type-1 regions.
      UInt64 maximumUsableAddress = defaultManagedBytes;
      UInt32 entryCount = 0;

      if (bootInfo) {
        entryCount = bootInfo->entryCount;

        if (entryCount > maxBootEntries) {
          entryCount = maxBootEntries;
        }
      }

      // determine highest usable address to manage (clip to 4 GB)
      if (bootInfo && entryCount > 0) {
        for (UInt32 i = 0; i < entryCount; ++i) {
          const MemoryRegion& region = bootInfo->entries[i];

          if (region.type != 1) {
            continue;
          }

          UInt64 baseAddress
            = (static_cast<UInt64>(region.baseHigh) << 32) | region.baseLow;
          UInt64 lengthBytes
            = (static_cast<UInt64>(region.lengthHigh) << 32) | region.lengthLow;

          if (lengthBytes == 0) continue;

          UInt64 endAddress  = baseAddress + lengthBytes;

          if (endAddress < baseAddress) continue; // overflow guard
          if (endAddress > maximumUsableAddress) {
            maximumUsableAddress = endAddress;
          }
        }
      }

      if (maximumUsableAddress == 0) {
        maximumUsableAddress = defaultManagedBytes;
      }

      if (maximumUsableAddress > 0xFFFFFFFFULL) {
        maximumUsableAddress = 0xFFFFFFFFULL;
      }

      managedBytes = static_cast<UInt32>(maximumUsableAddress & 0xFFFFFFFF);

      if (managedBytes < defaultManagedBytes) {
        managedBytes = defaultManagedBytes;
      }

      managedBytes = AlignUp(managedBytes, pageSize);
      pageCount = managedBytes / pageSize;

      UInt32 bitmapBytes = AlignUp((pageCount + 7) / 8, 4);
      UInt32 bitmapPhysical  = AlignUp(reinterpret_cast<UInt32>(&__bss_end), 4);

      pageBitmap = reinterpret_cast<UInt32*>(bitmapPhysical);
      bitmapLengthWords = bitmapBytes / 4;

      // default all pages to used
      for (UInt32 i = 0; i < bitmapLengthWords; ++i) {
        pageBitmap[i] = 0xFFFFFFFF;
      }

      UInt32 freePages = 0;

      // free usable pages from the map
      if (bootInfo && entryCount > 0) {
        for (UInt32 i = 0; i < entryCount; ++i) {
          const MemoryRegion& region = bootInfo->entries[i];

          if (region.type != 1) {
            continue;
          }

          UInt64 baseAddress
            = (static_cast<UInt64>(region.baseHigh) << 32) | region.baseLow;
          UInt64 lengthBytes
            = (static_cast<UInt64>(region.lengthHigh) << 32) | region.lengthLow;

          if (lengthBytes == 0) continue;

          UInt64 endAddress  = baseAddress + lengthBytes;

          if (endAddress < baseAddress) continue; // overflow

          // clip to 32-bit physical range we manage
          if (baseAddress >= 0x100000000ULL) continue;
          if (endAddress > 0x100000000ULL) endAddress = 0x100000000ULL;

          UInt32 startPage = static_cast<UInt32>(baseAddress / pageSize);
          UInt32 endPage
            = static_cast<UInt32>((endAddress + pageSize - 1) / pageSize);

          if (startPage >= pageCount) continue;
          if (endPage > pageCount) endPage = pageCount;

          for (UInt32 p = startPage; p < endPage; ++p) {
            ClearPageUsed(p);
            ++freePages;
          }
        }
      } else {
        // no map provided; treat all managed pages as free initially
        for (UInt32 i = 0; i < pageCount; ++i) {
          ClearPageUsed(i);
          ++freePages;
        }
      }

      // mark bitmap, kernel, page tables, boot info as used
      UInt32 usedUntil = AlignUp(bitmapPhysical + bitmapBytes, pageSize);
      UInt32 usedPages = usedUntil / pageSize;

      for (UInt32 i = 0; i < usedPages && i < pageCount; ++i) {
        SetPageUsed(i);
      }

      SetPageUsed(reinterpret_cast<UInt32>(pageDirectory) / pageSize);
      SetPageUsed(reinterpret_cast<UInt32>(firstPageTable) / pageSize);

      UInt32 bootInfoPage = bootInfoPhysicalAddress / pageSize;
      UInt32 bootInfoEndPage
        = (bootInfoPhysicalAddress + sizeof(BootInfo) + pageSize - 1)
        / pageSize;

      if (bootInfoPage < pageCount) {
        for (
          UInt32 p = bootInfoPage;
          p < bootInfoEndPage && p < pageCount;
          ++p
        ) {
          SetPageUsed(p);
        }
      }

      // never hand out the null page
      SetPageUsed(0);

      // reserve kernel image pages
      UInt32 kernelStart = reinterpret_cast<UInt32>(&__phys_start);
      UInt32 kernelEnd = AlignUp(reinterpret_cast<UInt32>(&__phys_end), pageSize);
      UInt32 kernelStartPage = kernelStart / pageSize;
      UInt32 kernelEndPage = kernelEnd / pageSize;

      if (kernelStartPage < pageCount) {
        if (kernelEndPage > pageCount) {
          kernelEndPage = pageCount;
        }

        for (UInt32 p = kernelStartPage; p < kernelEndPage; ++p) {
          SetPageUsed(p);
        }
      }

      // reserve early protected-mode stack pages (0x80000-0x90000)
      UInt32 stackBottom = 0x80000;
      UInt32 stackTop = 0x90000;
      UInt32 stackStartPage = stackBottom / pageSize;
      UInt32 stackEndPage = AlignUp(stackTop, pageSize) / pageSize;

      if (stackStartPage < pageCount) {
        if (stackEndPage > pageCount) {
          stackEndPage = pageCount;
        }

        for (UInt32 p = stackStartPage; p < stackEndPage; ++p) {
          SetPageUsed(p);
        }
      }

      // if nothing was free (bogus map), fall back to freeing everything except
      // reserved
      if (freePages == 0) {
        Logger::Write(
          LogLevel::Warning,
          "BootInfo memory map unusable; falling back to default map"
        );

        for (UInt32 i = 0; i < pageCount; ++i) {
          ClearPageUsed(i);
        }

        for (UInt32 i = 0; i < usedPages && i < pageCount; ++i) {
          SetPageUsed(i);
        }

        SetPageUsed(reinterpret_cast<UInt32>(pageDirectory) / pageSize);
        SetPageUsed(reinterpret_cast<UInt32>(firstPageTable) / pageSize);

        if (bootInfoPage < pageCount) {
          for (
            UInt32 p = bootInfoPage;
            p < bootInfoEndPage && p < pageCount;
            ++p
          ) {
            SetPageUsed(p);
          }
        }

        SetPageUsed(0);
      }

      // retain BootInfo for potential future diagnostics (not logged here)
    }

    /**
     * Allocates a single physical 4 KB page.
     * @return Physical address of the allocated page.
     */
    UInt32 AllocatePhysicalPage() {
      for (UInt32 i = 0; i < pageCount; ++i) {
        if (PageFree(i)) {
          SetPageUsed(i);
          return i * pageSize;
        }
      }

      PANIC("Out of physical memory");
      return 0;
    }
  }

  /**
   * Sets up identity paging for the managed physical range.
   * @param bootInfoPhysicalAddress Physical address of the boot info block.
   */
  void InitializePaging(UInt32 bootInfoPhysicalAddress) {
    InitializePhysicalAllocator(bootInfoPhysicalAddress);

    // clear directory and first table
    for (int i = 0; i < 1024; ++i) {
      pageDirectory[i] = 0;
      firstPageTable[i] = 0;
    }

    // identity map managedBytes and map kernel higher-half
    UInt32 tablesNeeded
      = (managedBytes + (4 * 1024 * 1024 - 1)) / (4 * 1024 * 1024);

    if (tablesNeeded > 1024) {
      tablesNeeded = 1024;
    }

    for (UInt32 tableIndex = 0; tableIndex < tablesNeeded; ++tableIndex) {
      UInt32* table;

      if (tableIndex == 0) {
        table = firstPageTable;
      } else {
        table = reinterpret_cast<UInt32*>(AllocatePhysicalPage());

        for (int i = 0; i < 1024; ++i) {
          table[i] = 0;
        }
      }

      UInt32 base = tableIndex * 1024 * pageSize;

      for (UInt32 i = 0; i < 1024; ++i) {
        table[i] = (base + i * pageSize) | pagePresent | pageWrite;
      }

      if (tableIndex == 0) {
        table[0] = 0; // guard null page
      }

      pageDirectory[tableIndex]
        = reinterpret_cast<UInt32>(table) | pagePresent | pageWrite;
    }

    // load directory and enable paging
    CPU::LoadPageDirectory(reinterpret_cast<UInt32>(pageDirectory));
    CPU::EnablePaging();

    // invalidate the null page TLB entry after enabling
    CPU::InvalidatePage(0);

  }

  void* AllocatePage() {
    UInt32 physicalPageAddress = AllocatePhysicalPage();

    return reinterpret_cast<void*>(physicalPageAddress);
  }

  void FreePage(void* physicalAddress) {
    UInt32 address = reinterpret_cast<UInt32>(physicalAddress);

    if (address % pageSize != 0) {
      Logger::Write(LogLevel::Warning, "FreePage: non-aligned address");
      return;
    }

    UInt32 index = address / pageSize;

    if (index >= pageCount) {
      Logger::Write(LogLevel::Warning, "FreePage: out-of-range page");
      return;
    }

    ClearPageUsed(index);
  }

  void MapPage(UInt32 virtualAddress, UInt32 physicalAddress, bool writable) {
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32* table;

    if (pageDirectory[pageDirectoryIndex] & pagePresent) {
      table = reinterpret_cast<UInt32*>(pageDirectory[pageDirectoryIndex] & ~0xFFF);
    } else {
      // allocate a new page table in low memory (identity mapped)
      table = reinterpret_cast<UInt32*>(AllocatePhysicalPage());

      for (int i = 0; i < 1024; ++i) {
        table[i] = 0;
      }

      pageDirectory[pageDirectoryIndex]
        = reinterpret_cast<UInt32>(table) | pagePresent | pageWrite;
    }

    UInt32 flags = pagePresent | (writable ? pageWrite : 0);
    table[pageTableIndex] = (physicalAddress & ~0xFFF) | flags;

    CPU::InvalidatePage(virtualAddress);
  }

  void UnmapPage(UInt32 virtualAddress) {
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

    if (!(pageDirectory[pageDirectoryIndex] & pagePresent)) {
      return;
    }

    UInt32* table
      = reinterpret_cast<UInt32*>(pageDirectory[pageDirectoryIndex] & ~0xFFF);
    table[pageTableIndex] = 0;

    CPU::InvalidatePage(virtualAddress);
  }
}
