/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Memory.cpp
 * IA32 paging and memory functions.
 */

#include <Arch/IA32/LinkerSymbols.hpp>
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Types/Boot/BootInfo.hpp>
#include <Types/Logging/Level.hpp>
#include <Types/Primitives.hpp>
#include <Types/Memory/MemoryRegion.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using BootInfo = Types::Boot::BootInfo;
  using LogLevel = Types::Logging::Level;
  using MemoryRegion = Types::Memory::MemoryRegion;

  namespace {
    /**
     * Size of a single page in bytes.
     */
    constexpr UInt32 _pageSize = 4096;

    /**
     * Number of entries per IA32 page directory.
     */
    constexpr UInt32 _pageDirectoryEntries = 1024;

    /**
     * Number of entries per IA32 page table.
     */
    constexpr UInt32 _pageTableEntries = 1024;

    /**
     * Page present bit.
     */
    constexpr UInt32 _pagePresent = 0x1;

    /**
     * Page writable bit.
     */
    constexpr UInt32 _pageWrite = 0x2;

    /**
     * Page user-accessible bit.
     */
    constexpr UInt32 _pageUser = 0x4;

    /**
     * Page global bit.
     */
    constexpr UInt32 _pageGlobal = 0x100;

    /**
     * Virtual address base exposing all page table entries via the recursive
     * slot.
     */
    constexpr UInt32 _recursivePageTablesBase = 0xFFC00000;

    /**
     * Virtual address exposing the page directory entries array via the
     * recursive slot.
     */
    constexpr UInt32 _recursivePageDirectory = 0xFFFFF000;

    /**
     * Maximum `BootInfo` entries to consume from firmware.
     */
    constexpr UInt32 _maxBootEntries = 32;

    /**
     * Default managed memory size when no `BootInfo` is present.
     */
    constexpr UInt32 _defaultManagedBytes = 64 * 1024 * 1024;

    /**
     * Total bytes under management by the physical allocator.
     */
    UInt32 _managedBytes = _defaultManagedBytes;

    /**
     * Total number of 4 KB pages under management.
     */
    UInt32 _pageCount = _defaultManagedBytes / _pageSize;

    /**
     * Number of pages currently marked used.
     */
    UInt32 _usedPages = 0;

    /**
     * Kernel page directory storage.
     */
    [[gnu::aligned(_pageSize)]]
    static UInt32 _pageDirectory[1024];

    /**
     * First page table used for initial identity mapping.
     */
    [[gnu::aligned(_pageSize)]]
    static UInt32 _firstPageTable[1024];

    /**
     * Bitmap tracking physical page usage.
     */
    static UInt32* _pageBitmap = nullptr;

    /**
     * Length of the page bitmap in 32-bit words.
     */
    static UInt32 _bitmapLengthWords = 0;

    /**
     * Allocates a single physical 4 KB page.
     * @param zero
     *   Whether to zero the page before returning it.
     * @return
     *   Physical address of the allocated page.
     */
    UInt32 AllocatePhysicalPage(bool zero);

    /**
     * Aligns a value down to the nearest alignment boundary.
     * @param value
     *   Value to align.
     * @param alignment
     *   Alignment boundary (power of two).
     * @return
     *   Aligned value at or below the input.
     */
    inline UInt32 AlignDown(UInt32 value, UInt32 alignment) {
      return value & ~(alignment - 1);
    }

    /**
     * Aligns a value up to the next alignment boundary.
     * @param value
     *   Value to align.
     * @param alignment
     *   Alignment boundary (power of two).
     * @return
     *   Aligned value at or above the input.
     */
    inline UInt32 AlignUp(UInt32 value, UInt32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }

    /**
     * Converts a kernel virtual address into its physical load address.
     * @param virtualAddress
     *   Higher-half virtual address.
     * @return
     *   Physical address corresponding to the loaded image.
     */
    inline UInt32 KernelVirtualToPhysical(UInt32 virtualAddress) {
      // all kernel segments are offset by kernelVirtualBase;
      // compute delta at runtime
      UInt32 kernelPhysicalBase = reinterpret_cast<UInt32>(&__phys_start);
      UInt32 kernelVirtualBase = reinterpret_cast<UInt32>(&__virt_start);

      if (virtualAddress >= Memory::kernelVirtualBase) {
        UInt32 offset = virtualAddress - kernelVirtualBase;

        return kernelPhysicalBase + offset;
      }

      return virtualAddress;
    }

    /**
     * Computes a bit mask for a specific bit index.
     * @param bit
     *   Bit index within the bitmap.
     * @return
     *   Mask with the indexed bit set.
     */
    inline UInt32 BitMask(UInt32 bit) {
      return (1u << (bit % 32));
    }

    /**
     * Converts a bit index to a bitmap word index.
     * @param bit
     *   Bit index within the bitmap.
     * @return
     *   Zero-based index of the 32-bit word containing the bit.
     */
    inline UInt32 BitmapWordIndex(UInt32 bit) {
      return bit / 32;
    }

    /**
     * Marks a page as used in the allocation bitmap.
     * @param pageIndex
     *   Page index to mark used.
     */
    inline void SetPageUsed(UInt32 pageIndex) {
      _pageBitmap[BitmapWordIndex(pageIndex)] |= BitMask(pageIndex);
    }

    /**
     * Marks a page as free in the allocation bitmap.
     * @param pageIndex
     *   Page index to mark free.
     */
    inline void ClearPageUsed(UInt32 pageIndex) {
      _pageBitmap[BitmapWordIndex(pageIndex)] &= ~BitMask(pageIndex);
    }

    /**
     * Tests whether a page is free according to the bitmap.
     * @param pageIndex
     *   Page index to query.
     * @return
     *   True if the page is free; false otherwise.
     */
    inline bool PageFree(UInt32 pageIndex) {
      return
        (_pageBitmap[BitmapWordIndex(pageIndex)]
        & BitMask(pageIndex)) == 0;
    }

    /**
     * Tests whether a page is marked used according to the bitmap.
     * @param pageIndex
     *   Page index to query.
     * @return
     *   True if the page is used; false otherwise.
     */
    inline bool PageUsed(UInt32 pageIndex) {
      return !PageFree(pageIndex);
    }

    /**
     * Finds the index of the first zero bit; returns -1 if none.
     * @param value
     *   32-bit word to scan.
     * @return
     *   Bit index [0,31] or -1 if all bits are one.
     */
    inline int FindFirstZeroBit(UInt32 value) {
      if (value == 0xFFFFFFFF) return -1;

      for (int i = 0; i < 32; ++i) {
        if ((value & (1u << i)) == 0) return i;
      }

      return -1;
    }

    /**
     * Returns a pointer to the page directory entry array via the recursive
     * mapping.
     * @return
     *   Pointer to the page-directory entries.
     */
    inline UInt32* GetPageDirectoryVirtual() {
      return reinterpret_cast<UInt32*>(_recursivePageDirectory);
    }

    /**
     * Returns a pointer to a page table entries array via the recursive
     * mapping.
     * @param pageDirectoryIndex
     *   Index of the page directory entry to project.
     * @return
     *   Pointer to the page-table entries for that page directory entry.
     */
    inline UInt32* GetPageTableVirtual(UInt32 pageDirectoryIndex) {
      return reinterpret_cast<UInt32*>(
        _recursivePageTablesBase + pageDirectoryIndex * _pageSize
      );
    }

    /**
     * Ensures a page table exists for a page directory entry index, allocating
     * if needed.
     * @param pageDirectoryIndex
     *   Index of the page directory entry to populate.
     * @return
     *   Pointer to the mapped page-table entries.
     */
    UInt32* EnsurePageTable(UInt32 pageDirectoryIndex) {
      if (_pageDirectory[pageDirectoryIndex] & _pagePresent) {
        // identity map keeps tables reachable even before higher-half switch
        return reinterpret_cast<UInt32*>(
          _pageDirectory[pageDirectoryIndex] & ~0xFFF
        );
      }

      UInt32 tablePhysical = 0;
      UInt32* table = nullptr;

      if (pageDirectoryIndex == 0) {
        // reuse the kernel's first page table (in .bss) for the first 4 MB
        table = _firstPageTable;
        tablePhysical
          = KernelVirtualToPhysical(reinterpret_cast<UInt32>(_firstPageTable));
      } else {
        tablePhysical = AllocatePhysicalPage(true);
        table = reinterpret_cast<UInt32*>(tablePhysical);

        for (UInt32 i = 0; i < _pageTableEntries; ++i) {
          table[i] = 0;
        }
      }

      _pageDirectory[pageDirectoryIndex]
        = tablePhysical
        | _pagePresent
        | _pageWrite;

      // return using the identity-mapped address
      // (physical == virtual in the low window).
      return reinterpret_cast<UInt32*>(tablePhysical);
    }

    /**
     * Initializes the physical page allocator using the provided boot info map.
     * @param bootInfoPhysicalAddress
     *   Physical address of the boot info block.
     */
    void InitializePhysicalAllocator(UInt32 bootInfoPhysicalAddress) {
      BootInfo* bootInfo = nullptr;

      if (
        bootInfoPhysicalAddress >= _pageSize &&
        bootInfoPhysicalAddress < _managedBytes
      ) {
        bootInfo = reinterpret_cast<BootInfo*>(bootInfoPhysicalAddress);
      }

      // track the highest usable address from type-1 regions
      UInt64 maximumUsableAddress = _defaultManagedBytes;
      UInt32 entryCount = 0;

      if (bootInfo) {
        entryCount = bootInfo->EntryCount;

        if (entryCount > _maxBootEntries) {
          entryCount = _maxBootEntries;
        }
      }

      // determine highest usable address to manage (clip to 4 gb)
      if (bootInfo && entryCount > 0) {
        for (UInt32 i = 0; i < entryCount; ++i) {
          const MemoryRegion& region = bootInfo->Entries[i];

          if (region.Type != 1) {
            continue;
          }

          UInt64 baseAddress
            = (static_cast<UInt64>(region.BaseHigh) << 32) | region.BaseLow;
          UInt64 lengthBytes
            = (static_cast<UInt64>(region.LengthHigh) << 32) | region.LengthLow;

          if (lengthBytes == 0) continue;

          UInt64 endAddress  = baseAddress + lengthBytes;

          if (endAddress < baseAddress) continue; // overflow guard
          if (endAddress > maximumUsableAddress) {
            maximumUsableAddress = endAddress;
          }
        }
      }

      if (maximumUsableAddress == 0) {
        maximumUsableAddress = _defaultManagedBytes;
      }

      if (maximumUsableAddress > 0xFFFFFFFFULL) {
        maximumUsableAddress = 0xFFFFFFFFULL;
      }

      _managedBytes = static_cast<UInt32>(maximumUsableAddress & 0xFFFFFFFF);

      if (_managedBytes < _defaultManagedBytes) {
        _managedBytes = _defaultManagedBytes;
      }

      _managedBytes = AlignUp(_managedBytes, _pageSize);
      _pageCount = _managedBytes / _pageSize;

      UInt32 bitmapBytes = AlignUp((_pageCount + 7) / 8, 4);
      UInt32 bitmapPhysical
        = AlignUp(reinterpret_cast<UInt32>(&__phys_bss_end), 4);

      _pageBitmap = reinterpret_cast<UInt32*>(bitmapPhysical);
      _bitmapLengthWords = bitmapBytes / 4;

      // default all pages to used
      for (UInt32 i = 0; i < _bitmapLengthWords; ++i) {
        _pageBitmap[i] = 0xFFFFFFFF;
      }

      UInt32 freePages = 0;

      // free usable pages from the map
      if (bootInfo && entryCount > 0) {
        for (UInt32 i = 0; i < entryCount; ++i) {
          const MemoryRegion& region = bootInfo->Entries[i];

          if (region.Type != 1) {
            continue;
          }

          UInt64 baseAddress
            = (static_cast<UInt64>(region.BaseHigh) << 32) | region.BaseLow;
          UInt64 lengthBytes
            = (static_cast<UInt64>(region.LengthHigh) << 32) | region.LengthLow;

          if (lengthBytes == 0) continue;

          UInt64 endAddress  = baseAddress + lengthBytes;

          if (endAddress < baseAddress) continue; // overflow

          // clip to 32-bit physical range we manage
          if (baseAddress >= 0x100000000ULL) continue;
          if (endAddress > 0x100000000ULL) endAddress = 0x100000000ULL;

          UInt32 startPage = static_cast<UInt32>(baseAddress / _pageSize);
          UInt32 endPage
            = static_cast<UInt32>((endAddress + _pageSize - 1) / _pageSize);

          if (startPage >= _pageCount) continue;
          if (endPage > _pageCount) endPage = _pageCount;

          for (UInt32 p = startPage; p < endPage; ++p) {
            ClearPageUsed(p);
            ++freePages;
          }
        }
      } else {
        // no map provided; treat all managed pages as free initially
        for (UInt32 i = 0; i < _pageCount; ++i) {
          ClearPageUsed(i);
          ++freePages;
        }
      }

      // mark bitmap, kernel, page tables, boot info as used
      UInt32 usedUntil = AlignUp(bitmapPhysical + bitmapBytes, _pageSize);
      UInt32 usedPages = usedUntil / _pageSize;

      for (UInt32 i = 0; i < usedPages && i < _pageCount; ++i) {
        SetPageUsed(i);
      }

      SetPageUsed(
        KernelVirtualToPhysical(reinterpret_cast<UInt32>(_pageDirectory))
        / _pageSize
      );
      SetPageUsed(
        KernelVirtualToPhysical(reinterpret_cast<UInt32>(_firstPageTable))
        / _pageSize
      );

      UInt32 bootInfoPage = bootInfoPhysicalAddress / _pageSize;
      UInt32 bootInfoEndPage
        = (bootInfoPhysicalAddress + sizeof(BootInfo) + _pageSize - 1)
        / _pageSize;

      if (bootInfoPage < _pageCount) {
        for (
          UInt32 p = bootInfoPage;
          p < bootInfoEndPage && p < _pageCount;
          ++p
        ) {
          SetPageUsed(p);
        }
      }

      // never hand out the null page
      SetPageUsed(0);

      // reserve kernel image pages
      UInt32 kernelStart = reinterpret_cast<UInt32>(&__phys_start);
      UInt32 kernelEnd
        = AlignUp(reinterpret_cast<UInt32>(&__phys_end), _pageSize);
      UInt32 kernelStartPage = kernelStart / _pageSize;
      UInt32 kernelEndPage = kernelEnd / _pageSize;

      if (kernelStartPage < _pageCount) {
        if (kernelEndPage > _pageCount) {
          kernelEndPage = _pageCount;
        }

        for (UInt32 p = kernelStartPage; p < kernelEndPage; ++p) {
          SetPageUsed(p);
        }
      }

      // reserve early protected-mode stack pages (0x80000-0x90000)
      UInt32 stackBottom = 0x80000;
      UInt32 stackTop = 0x90000;
      UInt32 stackStartPage = stackBottom / _pageSize;
      UInt32 stackEndPage = AlignUp(stackTop, _pageSize) / _pageSize;

      if (stackStartPage < _pageCount) {
        if (stackEndPage > _pageCount) {
          stackEndPage = _pageCount;
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

        for (UInt32 i = 0; i < _pageCount; ++i) {
          ClearPageUsed(i);
        }

        for (UInt32 i = 0; i < usedPages && i < _pageCount; ++i) {
          SetPageUsed(i);
        }

        SetPageUsed(
          KernelVirtualToPhysical(reinterpret_cast<UInt32>(_pageDirectory))
          / _pageSize
        );
        SetPageUsed(
          KernelVirtualToPhysical(reinterpret_cast<UInt32>(_firstPageTable))
          / _pageSize
        );

        if (bootInfoPage < _pageCount) {
          for (
            UInt32 p = bootInfoPage;
            p < bootInfoEndPage && p < _pageCount;
            ++p
          ) {
            SetPageUsed(p);
          }
        }

        SetPageUsed(0);

        freePages = 0;
        for (UInt32 i = 0; i < _pageCount; ++i) {
          if (PageFree(i)) {
            ++freePages;
          }
        }
      }

      freePages = 0;
      for (UInt32 i = 0; i < _pageCount; ++i) {
        if (PageFree(i)) {
          ++freePages;
        }
      }

      usedPages = _pageCount - freePages;

      // retain BootInfo for potential future diagnostics (not logged here)
    }

    /**
     * Allocates a single physical 4 KB page.
     * @param zero
     *   Whether to zero the page before returning it.
     * @return
     *   Physical address of the allocated page.
     */
    UInt32 AllocatePhysicalPage(bool zero) {
      UInt32 words = _bitmapLengthWords;

      for (UInt32 wordIndex = 0; wordIndex < words; ++wordIndex) {
        UInt32 word = _pageBitmap[wordIndex];
        int bit = FindFirstZeroBit(word);

        if (bit >= 0) {
          UInt32 pageIndex = wordIndex * 32u + static_cast<UInt32>(bit);

          if (pageIndex >= _pageCount) break;

          SetPageUsed(pageIndex);
          ++_usedPages;

          if (zero) {
            UInt8* memory = reinterpret_cast<UInt8*>(pageIndex * _pageSize);
            for (UInt32 b = 0; b < _pageSize; ++b) {
              memory[b] = 0;
            }
          }

          return pageIndex * _pageSize;
        }
      }

      PANIC("Out of physical memory");

      return 0;
    }
  }

  void Memory::InitializePaging(UInt32 bootInfoPhysicalAddress) {
    InitializePhysicalAllocator(bootInfoPhysicalAddress);

    // clear directory and first table
    for (int i = 0; i < static_cast<int>(_pageDirectoryEntries); ++i) {
      _pageDirectory[i] = 0;
      _firstPageTable[i] = 0;
    }

    // identity map managedBytes (keep identity window for now)
    UInt32 tablesNeeded
      = (_managedBytes + (4 * 1024 * 1024 - 1)) / (4 * 1024 * 1024);

    if (tablesNeeded > 1024) {
      tablesNeeded = 1024;
    }

    for (UInt32 tableIndex = 0; tableIndex < tablesNeeded; ++tableIndex) {
      UInt32* table = EnsurePageTable(tableIndex);

      UInt32 base = tableIndex * _pageTableEntries * _pageSize;

      for (UInt32 i = 0; i < _pageTableEntries; ++i) {
        table[i]
          = (base + i * _pageSize) | _pagePresent | _pageWrite | _pageGlobal;
      }

      if (tableIndex == 0) {
        table[0] = 0; // guard null page
      }

      _pageDirectory[tableIndex]
        = reinterpret_cast<UInt32>(table) | _pagePresent | _pageWrite;
    }

    // map the kernel image into the higher half
    UInt32 kernelPhysicalStart = reinterpret_cast<UInt32>(&__phys_start);
    UInt32 kernelPhysicalEnd = reinterpret_cast<UInt32>(&__phys_end);
    UInt32 kernelSizeBytes = kernelPhysicalEnd - kernelPhysicalStart;

    for (UInt32 offset = 0; offset < kernelSizeBytes; offset += _pageSize) {
      UInt32 physicalAddress = kernelPhysicalStart + offset;
      UInt32 virtualAddress = kernelVirtualBase + offset;

      MapPage(virtualAddress, physicalAddress, true, false, true);
    }

    // install recursive mapping in the last PDE
    UInt32 pageDirectoryPhysical = KernelVirtualToPhysical(
      reinterpret_cast<UInt32>(_pageDirectory)
    );

    _pageDirectory[Memory::recursiveSlot]
      = pageDirectoryPhysical | _pagePresent | _pageWrite;

    // load directory and enable paging,
    // invalidate the null page TLB entry after enabling
    CPU::LoadPageDirectory(pageDirectoryPhysical);
    CPU::EnablePaging();
    CPU::InvalidatePage(0);

  }

  void* Memory::AllocatePage(bool zero) {
    UInt32 physicalPageAddress = AllocatePhysicalPage(zero);

    return reinterpret_cast<void*>(physicalPageAddress);
  }

  void Memory::FreePage(void* physicalAddress) {
    UInt32 address = reinterpret_cast<UInt32>(physicalAddress);

    if (address % _pageSize != 0) {
      Logger::Write(LogLevel::Warning, "FreePage: non-aligned address");
      return;
    }

    UInt32 index = address / _pageSize;

    if (index >= _pageCount) {
      Logger::Write(LogLevel::Warning, "FreePage: out-of-range page");
      return;
    }

    if (PageFree(index)) {
      Logger::Write(LogLevel::Warning, "FreePage: double free detected");
      return;
    }

    ClearPageUsed(index);

    if (_usedPages > 0) {
      --_usedPages;
    }
  }

  void Memory::MapPage(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32* table = EnsurePageTable(pageDirectoryIndex);
    UInt32 flags = _pagePresent
      | (writable ? _pageWrite : 0)
      | (user ? _pageUser : 0)
      | (global ? _pageGlobal : 0);

    table[pageTableIndex] = (physicalAddress & ~0xFFF) | flags;

    CPU::InvalidatePage(virtualAddress);
  }

  void Memory::UnmapPage(UInt32 virtualAddress) {
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;

    if (!(_pageDirectory[pageDirectoryIndex] & _pagePresent)) {
      return;
    }

    UInt32* table = reinterpret_cast<UInt32*>(
      _pageDirectory[pageDirectoryIndex] & ~0xFFF
    );

    table[pageTableIndex] = 0;

    CPU::InvalidatePage(virtualAddress);
  }

  void Memory::MapRange(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    UInt32 lengthBytes,
    bool writable,
    bool user,
    bool global
  ) {
    UInt32 bytes = AlignUp(lengthBytes, _pageSize);
    UInt32 count = bytes / _pageSize;

    for (UInt32 i = 0; i < count; ++i) {
      MapPage(
        virtualAddress + i * _pageSize,
        physicalAddress + i * _pageSize,
        writable,
        user,
        global
      );
    }
  }

  void Memory::UnmapRange(UInt32 virtualAddress, UInt32 lengthBytes) {
    UInt32 bytes = AlignUp(lengthBytes, _pageSize);
    UInt32 count = bytes / _pageSize;

    for (UInt32 i = 0; i < count; ++i) {
      UnmapPage(virtualAddress + i * _pageSize);
    }
  }

  void Memory::ReservePhysicalRange(
    UInt32 physicalAddress,
    UInt32 lengthBytes
  ) {
    UInt32 start = AlignDown(physicalAddress, _pageSize);
    UInt32 end = AlignUp(physicalAddress + lengthBytes, _pageSize);
    UInt32 startPage = start / _pageSize;
    UInt32 endPage = end / _pageSize;

    if (endPage > _pageCount) {
      endPage = _pageCount;
    }

    for (UInt32 p = startPage; p < endPage; ++p) {
      if (PageFree(p)) {
        SetPageUsed(p);

        ++_usedPages;
      }
    }
  }

  void Memory::ReleasePhysicalRange(
    UInt32 physicalAddress,
    UInt32 lengthBytes
  ) {
    UInt32 start = AlignDown(physicalAddress, _pageSize);
    UInt32 end = AlignUp(physicalAddress + lengthBytes, _pageSize);
    UInt32 startPage = start / _pageSize;
    UInt32 endPage = end / _pageSize;

    if (endPage > _pageCount) {
      endPage = _pageCount;
    }

    for (UInt32 p = startPage; p < endPage; ++p) {
      if (PageUsed(p)) {
        ClearPageUsed(p);
        if (_usedPages > 0) {
          --_usedPages;
        }
      }
    }
  }

  UInt32 Memory::GetPageDirectoryEntry(UInt32 virtualAddress) {
    UInt32 index = (virtualAddress >> 22) & 0x3FF;
    UInt32* directory = GetPageDirectoryVirtual();

    return directory[index];
  }

  UInt32 Memory::GetPageTableEntry(UInt32 virtualAddress) {
    UInt32 directoryEntry = GetPageDirectoryEntry(virtualAddress);

    if ((directoryEntry & _pagePresent) == 0) {
      return 0;
    }

    UInt32 tableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32* table = GetPageTableVirtual((virtualAddress >> 22) & 0x3FF);

    return table[tableIndex];
  }

  Memory::PhysicalAllocatorState Memory::GetPhysicalAllocatorState() {
    PhysicalAllocatorState state{};

    state.TotalPages = _pageCount;
    state.UsedPages = _usedPages;
    state.FreePages = _pageCount - _usedPages;

    return state;
  }
}
