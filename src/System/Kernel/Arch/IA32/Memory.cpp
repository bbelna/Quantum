//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Memory.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 paging setup and simple physical page allocator.
//------------------------------------------------------------------------------

#include <Arch/IA32/LinkerSymbols.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Memory.hpp>
#include <BootInfo.hpp>
#include <Drivers/Console.hpp>
#include <Kernel.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  namespace {
    constexpr uint32 pageSize = 4096;
    constexpr uint32 pagePresent = 0x1;
    constexpr uint32 pageWrite = 0x2;
    constexpr uint32 maxBootEntries = 32;
    constexpr uint32 defaultManagedBytes = 64 * 1024 * 1024;

    uint32 managedBytes = defaultManagedBytes;
    uint32 pageCount = defaultManagedBytes / pageSize;

    static uint32 pageDirectory[1024] __attribute__((aligned(pageSize)));
    static uint32 firstPageTable[1024] __attribute__((aligned(pageSize)));
    static uint32* pageBitmap = nullptr;
    static uint32 bitmapLengthWords = 0;

    /**
     * Aligns a value up to the specified alignment boundary.
     * @param value Value to align.
     * @param alignment Alignment to apply (must be power of two).
     * @return Aligned value.
     */
    inline uint32 AlignUp(uint32 value, uint32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }

    /**
     * Marks a page as used in the bitmap.
     * @param pageIndex Index of the page to mark.
     */
    inline void SetPageUsed(uint32 pageIndex) {
      pageBitmap[pageIndex / 32] |= (1u << (pageIndex % 32));
    }

    /**
     * Clears the used bit for a page in the bitmap.
     * @param pageIndex Index of the page to clear.
     */
    inline void ClearPageUsed(uint32 pageIndex) {
      pageBitmap[pageIndex / 32] &= ~(1u << (pageIndex % 32));
    }

    /**
     * Tests whether a page is free in the bitmap.
     * @param pageIndex Index of the page to test.
     * @return true if the page is free; false otherwise.
     */
    inline bool PageFree(uint32 pageIndex) {
      return (pageBitmap[pageIndex / 32] & (1u << (pageIndex % 32))) == 0;
    }

    /**
     * Initializes the physical page allocator using the provided boot info map.
     * @param bootInfoPhysicalAddress Physical address of the boot info block.
     */
    void InitializePhysicalAllocator(uint32 bootInfoPhysicalAddress) {
      BootInfo* bootInfo = nullptr;

      if (
        bootInfoPhysicalAddress >= pageSize &&
        bootInfoPhysicalAddress < managedBytes
      ) {
        bootInfo = reinterpret_cast<BootInfo*>(bootInfoPhysicalAddress);
      }

      // Track the highest usable address from type-1 regions.
      uint64 maximumUsableAddress = defaultManagedBytes;
      uint32 entryCount = 0;

      if (bootInfo) {
        entryCount = bootInfo->entryCount;

        if (entryCount > maxBootEntries) {
          entryCount = maxBootEntries;
        }
      }

      // determine highest usable address to manage (clip to 4 GB)
      if (bootInfo && entryCount > 0) {
        for (uint32 i = 0; i < entryCount; ++i) {
          const MemoryRegion& region = bootInfo->entries[i];

          if (region.type != 1) {
            continue;
          }

          uint64 baseAddress
            = (static_cast<uint64>(region.baseHigh) << 32) | region.baseLow;
          uint64 lengthBytes
            = (static_cast<uint64>(region.lengthHigh) << 32) | region.lengthLow;

          if (lengthBytes == 0) continue;

          uint64 endAddress  = baseAddress + lengthBytes;

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

      managedBytes = static_cast<uint32>(maximumUsableAddress & 0xFFFFFFFF);

      if (managedBytes < defaultManagedBytes) {
        managedBytes = defaultManagedBytes;
      }

      managedBytes = AlignUp(managedBytes, pageSize);
      pageCount = managedBytes / pageSize;

      uint32 bitmapBytes = AlignUp((pageCount + 7) / 8, 4);
      uint32 bitmapPhysical  = AlignUp(reinterpret_cast<uint32>(&__bss_end), 4);

      pageBitmap = reinterpret_cast<uint32*>(bitmapPhysical);
      bitmapLengthWords = bitmapBytes / 4;

      // default all pages to used
      for (uint32 i = 0; i < bitmapLengthWords; ++i) {
        pageBitmap[i] = 0xFFFFFFFF;
      }

      uint32 freePages = 0;

      // free usable pages from the map
      if (bootInfo && entryCount > 0) {
        for (uint32 i = 0; i < entryCount; ++i) {
          const MemoryRegion& region = bootInfo->entries[i];

          if (region.type != 1) {
            continue;
          }

          uint64 baseAddress
            = (static_cast<uint64>(region.baseHigh) << 32) | region.baseLow;
          uint64 lengthBytes
            = (static_cast<uint64>(region.lengthHigh) << 32) | region.lengthLow;

          if (lengthBytes == 0) continue;

          uint64 endAddress  = baseAddress + lengthBytes;

          if (endAddress < baseAddress) continue; // overflow

          // clip to 32-bit physical range we manage
          if (baseAddress >= 0x100000000ULL) continue;
          if (endAddress > 0x100000000ULL) endAddress = 0x100000000ULL;

          uint32 startPage = static_cast<uint32>(baseAddress / pageSize);
          uint32 endPage
            = static_cast<uint32>((endAddress + pageSize - 1) / pageSize);

          if (startPage >= pageCount) continue;
          if (endPage > pageCount) endPage = pageCount;

          for (uint32 p = startPage; p < endPage; ++p) {
            ClearPageUsed(p);
            ++freePages;
          }
        }
      } else {
        // no map provided; treat all managed pages as free initially
        for (uint32 i = 0; i < pageCount; ++i) {
          ClearPageUsed(i);
          ++freePages;
        }
      }

      // mark bitmap, kernel, page tables, boot info as used
      uint32 usedUntil = AlignUp(bitmapPhysical + bitmapBytes, pageSize);
      uint32 usedPages = usedUntil / pageSize;

      for (uint32 i = 0; i < usedPages && i < pageCount; ++i) {
        SetPageUsed(i);
      }

      SetPageUsed(reinterpret_cast<uint32>(pageDirectory) / pageSize);
      SetPageUsed(reinterpret_cast<uint32>(firstPageTable) / pageSize);

      uint32 bootInfoPage = bootInfoPhysicalAddress / pageSize;
      uint32 bootInfoEndPage
        = (bootInfoPhysicalAddress + sizeof(BootInfo) + pageSize - 1)
        / pageSize;

      if (bootInfoPage < pageCount) {
        for (
          uint32 p = bootInfoPage;
          p < bootInfoEndPage && p < pageCount;
          ++p
        ) {
          SetPageUsed(p);
        }
      }

      // never hand out the null page
      SetPageUsed(0);

      // reserve kernel image pages
      uint32 kernelStart = reinterpret_cast<uint32>(&__phys_start);
      uint32 kernelEnd = AlignUp(reinterpret_cast<uint32>(&__phys_end), pageSize);
      uint32 kernelStartPage = kernelStart / pageSize;
      uint32 kernelEndPage = kernelEnd / pageSize;

      if (kernelStartPage < pageCount) {
        if (kernelEndPage > pageCount) {
          kernelEndPage = pageCount;
        }

        for (uint32 p = kernelStartPage; p < kernelEndPage; ++p) {
          SetPageUsed(p);
        }
      }

      // reserve early protected-mode stack pages (0x80000-0x90000)
      uint32 stackBottom = 0x80000;
      uint32 stackTop = 0x90000;
      uint32 stackStartPage = stackBottom / pageSize;
      uint32 stackEndPage = AlignUp(stackTop, pageSize) / pageSize;

      if (stackStartPage < pageCount) {
        if (stackEndPage > pageCount) {
          stackEndPage = pageCount;
        }

        for (uint32 p = stackStartPage; p < stackEndPage; ++p) {
          SetPageUsed(p);
        }
      }

      // if nothing was free (bogus map), fall back to freeing everything except
      // reserved
      if (freePages == 0) {
        Drivers::Console::WriteLine(
          "BootInfo memory map unusable; falling back to default map"
        );

        for (uint32 i = 0; i < pageCount; ++i) {
          ClearPageUsed(i);
        }

        for (uint32 i = 0; i < usedPages && i < pageCount; ++i) {
          SetPageUsed(i);
        }

        SetPageUsed(reinterpret_cast<uint32>(pageDirectory) / pageSize);
        SetPageUsed(reinterpret_cast<uint32>(firstPageTable) / pageSize);

        if (bootInfoPage < pageCount) {
          for (
            uint32 p = bootInfoPage;
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
    uint32 AllocatePhysicalPage() {
      for (uint32 i = 0; i < pageCount; ++i) {
        if (PageFree(i)) {
          SetPageUsed(i);
          return i * pageSize;
        }
      }

      Kernel::Panic("Out of physical memory", __FILE__, __LINE__, __func__);
      return 0;
    }
  }

  /**
   * Sets up identity paging for the managed physical range.
   * @param bootInfoPhysicalAddress Physical address of the boot info block.
   */
  void InitializePaging(uint32 bootInfoPhysicalAddress) {
    InitializePhysicalAllocator(bootInfoPhysicalAddress);

    // clear directory and first table
    for (int i = 0; i < 1024; ++i) {
      pageDirectory[i] = 0;
      firstPageTable[i] = 0;
    }

    // identity map managedBytes and map kernel higher-half
    uint32 tablesNeeded
      = (managedBytes + (4 * 1024 * 1024 - 1)) / (4 * 1024 * 1024);

    if (tablesNeeded > 1024) {
      tablesNeeded = 1024;
    }

    for (uint32 tableIndex = 0; tableIndex < tablesNeeded; ++tableIndex) {
      uint32* table;

      if (tableIndex == 0) {
        table = firstPageTable;
      } else {
        table = reinterpret_cast<uint32*>(AllocatePhysicalPage());

        for (int i = 0; i < 1024; ++i) {
          table[i] = 0;
        }
      }

      uint32 base = tableIndex * 1024 * pageSize;

      for (uint32 i = 0; i < 1024; ++i) {
        table[i] = (base + i * pageSize) | pagePresent | pageWrite;
      }

      if (tableIndex == 0) {
        table[0] = 0; // guard null page
      }

      pageDirectory[tableIndex]
        = reinterpret_cast<uint32>(table) | pagePresent | pageWrite;
    }

    // load directory and enable paging
    CPU::LoadPageDirectory(reinterpret_cast<uint32>(pageDirectory));
    CPU::EnablePaging();

    // invalidate the null page TLB entry after enabling
    CPU::InvalidatePage(0);

  }

  void* AllocatePage() {
    uint32 physicalPageAddress = AllocatePhysicalPage();

    return reinterpret_cast<void*>(physicalPageAddress);
  }

  void FreePage(void* physicalAddress) {
    uint32 address = reinterpret_cast<uint32>(physicalAddress);

    if (address % pageSize != 0) {
      Drivers::Console::WriteLine("FreePage: non-aligned address");
      return;
    }

    uint32 index = address / pageSize;

    if (index >= pageCount) {
      Drivers::Console::WriteLine("FreePage: out-of-range page");
      return;
    }

    ClearPageUsed(index);
  }

  void MapPage(uint32 virtualAddress, uint32 physicalAddress, bool writable) {
    uint32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    uint32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;
    uint32* table;

    if (pageDirectory[pageDirectoryIndex] & pagePresent) {
      table = reinterpret_cast<uint32*>(pageDirectory[pageDirectoryIndex] & ~0xFFF);
    } else {
      // allocate a new page table in low memory (identity mapped)
      table = reinterpret_cast<uint32*>(AllocatePhysicalPage());

      for (int i = 0; i < 1024; ++i) {
        table[i] = 0;
      }

      pageDirectory[pageDirectoryIndex]
        = reinterpret_cast<uint32>(table) | pagePresent | pageWrite;
    }

    uint32 flags = pagePresent | (writable ? pageWrite : 0);
    table[pageTableIndex] = (physicalAddress & ~0xFFF) | flags;

    CPU::InvalidatePage(virtualAddress);
  }

  void UnmapPage(uint32 virtualAddress) {
    uint32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    uint32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

    if (!(pageDirectory[pageDirectoryIndex] & pagePresent)) {
      return;
    }

    uint32* table
      = reinterpret_cast<uint32*>(pageDirectory[pageDirectoryIndex] & ~0xFFF);
    table[pageTableIndex] = 0;

    CPU::InvalidatePage(virtualAddress);
  }
}
