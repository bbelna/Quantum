/**
 * @file System/Kernel/Arch/IA32/Memory.cpp
 * @brief IA32 memory and paging management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Align.hpp>
#include <Types.hpp>

#include "Arch/IA32/BootInfo.hpp"
#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Interrupts.hpp"
#include "Arch/IA32/LinkerSymbols.hpp"
#include "Arch/IA32/Memory.hpp"
#include "Logger.hpp"
#include "Panic.hpp"
#include "Prelude.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Kernel::Logger::Level;
  using ::Quantum::AlignDown;
  using ::Quantum::AlignUp;

  alignas(Memory::_pageSize)
  UInt32 Memory::_pageDirectory[Memory::_pageDirectoryEntries];

  alignas(Memory::_pageSize)
  UInt32 Memory::_firstPageTable[Memory::_pageTableEntries];

  UInt32 Memory::KernelVirtualToPhysical(UInt32 virtualAddress) {
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

  UInt32 Memory::BitMask(UInt32 bit) {
    return (1u << (bit % 32));
  }

  UInt32 Memory::BitmapWordIndex(UInt32 bit) {
    return bit / 32;
  }

  void Memory::SetPageUsed(UInt32 pageIndex) {
    _pageBitmap[BitmapWordIndex(pageIndex)] |= BitMask(pageIndex);
  }

  void Memory::ClearPageUsed(UInt32 pageIndex) {
    _pageBitmap[BitmapWordIndex(pageIndex)] &= ~BitMask(pageIndex);
  }

  bool Memory::PageFree(UInt32 pageIndex) {
    return
      (_pageBitmap[BitmapWordIndex(pageIndex)]
      & BitMask(pageIndex)) == 0;
  }

  bool Memory::PageUsed(UInt32 pageIndex) {
    return !PageFree(pageIndex);
  }

  int Memory::FindFirstZeroBit(UInt32 value) {
    if (value == 0xFFFFFFFF) return -1;

    for (int i = 0; i < 32; ++i) {
      if ((value & (1u << i)) == 0) return i;
    }

    return -1;
  }

  UInt32* Memory::GetPageDirectoryVirtual() {
    return reinterpret_cast<UInt32*>(_recursivePageDirectory);
  }

  UInt32* Memory::GetPageTableVirtual(UInt32 pageDirectoryIndex) {
    return reinterpret_cast<UInt32*>(
      _recursivePageTablesBase + pageDirectoryIndex * _pageSize
    );
  }

  UInt32* Memory::EnsurePageTable(UInt32 pageDirectoryIndex) {
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

  void Memory::EnsureKernelHeapTables() {
    constexpr UInt32 heapBase = Memory::kernelHeapBase;
    constexpr UInt32 heapBytes = Memory::kernelHeapBytes;
    UInt32 startIndex = heapBase >> 22;
    UInt32 endIndex = (heapBase + heapBytes - 1) >> 22;

    for (UInt32 index = startIndex; index <= endIndex; ++index) {
      EnsurePageTable(index);
    }
  }

  void Memory::InitializePhysicalAllocator(UInt32 bootInfoPhysicalAddress) {
    const BootInfo::View* bootInfo = nullptr;
    UInt32 bootInfoPhysical = BootInfo::GetPhysicalAddress();

    if (bootInfoPhysical == 0) {
      bootInfoPhysical = bootInfoPhysicalAddress;
    }

    if (bootInfoPhysical >= _pageSize && bootInfoPhysical < _managedBytes) {
      bootInfo = BootInfo::Get();
    }

    // track the highest usable address from type-1 regions
    UInt64 maximumUsableAddress = _defaultManagedBytes;
    UInt32 entryCount = 0;

    if (bootInfo) {
      entryCount = bootInfo->entryCount;

      if (entryCount > _maxBootEntries) {
        entryCount = _maxBootEntries;
      }
    }

    // determine highest usable address to manage (clip to 4 gb)
    if (bootInfo && entryCount > 0) {
      for (UInt32 i = 0; i < entryCount; ++i) {
        const BootInfo::Region& region = bootInfo->entries[i];

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

    if (bootInfo && bootInfo->initBundleSize > 0) {
      UInt32 bundleStart = bootInfo->initBundlePhysical;
      UInt32 bundleEnd = bundleStart + bootInfo->initBundleSize;
      UInt32 bitmapEnd = bitmapPhysical + bitmapBytes;

      if (!(bitmapEnd <= bundleStart || bitmapPhysical >= bundleEnd)) {
        bitmapPhysical = AlignUp(bundleEnd, 4);
      }
    }

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
        const BootInfo::Region& region = bootInfo->entries[i];

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

    UInt32 bootInfoPage = bootInfoPhysical / _pageSize;
    UInt32 bootInfoEndPage
      = (bootInfoPhysical + BootInfo::rawSize + _pageSize - 1)
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

    if (bootInfo && bootInfo->initBundleSize > 0) {
      Memory::ReservePhysicalRange(
        bootInfo->initBundlePhysical,
        bootInfo->initBundleSize
      );

      UInt32 bundleStart = AlignDown(
        bootInfo->initBundlePhysical,
        _pageSize
      );
      UInt32 bundleEnd = AlignUp(
        bootInfo->initBundlePhysical + bootInfo->initBundleSize,
        _pageSize
      );
      _initBundleStartPage = bundleStart / _pageSize;
      _initBundleEndPage = bundleEnd / _pageSize;

      Logger::WriteFormatted(
        LogLevel::Debug,
        "INIT.BND reserve pages %u-%u (phys=%p size=%p)",
        _initBundleStartPage,
        _initBundleEndPage,
        bootInfo->initBundlePhysical,
        bootInfo->initBundleSize
      );
    } else {
      _initBundleStartPage = 0;
      _initBundleEndPage = 0;
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

  UInt32 Memory::AllocatePhysicalPage(bool zero) {
    UInt32 words = _bitmapLengthWords;

    for (UInt32 wordIndex = 0; wordIndex < words; ++wordIndex) {
      UInt32 word = _pageBitmap[wordIndex];

      while (true) {
        int bit = FindFirstZeroBit(word);

        if (bit < 0) {
          break;
        }

        UInt32 pageIndex = wordIndex * 32u + static_cast<UInt32>(bit);

        if (pageIndex >= _pageCount) {
          break;
        }

        if (
          _initBundleEndPage > _initBundleStartPage
          && pageIndex >= _initBundleStartPage
          && pageIndex < _initBundleEndPage
        ) {
          SetPageUsed(pageIndex);
          ++_usedPages;

          if (!_loggedBundleSkip) {
            Logger::WriteFormatted(
              LogLevel::Warning,
              "AllocatePhysicalPage: skipping INIT.BND page %u",
              pageIndex
            );
            _loggedBundleSkip = true;
          }

          word = _pageBitmap[wordIndex];
          continue;
        }

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

    EnsureKernelHeapTables();

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

  void* Memory::AllocatePageBelow(
    UInt32 maxPhysicalAddress,
    bool zero,
    UInt32 boundaryBytes
  ) {
    if (maxPhysicalAddress == 0) {
      return nullptr;
    }

    UInt32 maxPage = maxPhysicalAddress / _pageSize;

    if (maxPage > _pageCount) {
      maxPage = _pageCount;
    }

    for (UInt32 pageIndex = 0; pageIndex < maxPage; ++pageIndex) {
      if (!PageFree(pageIndex)) {
        continue;
      }

      UInt32 physical = pageIndex * _pageSize;

      if (boundaryBytes != 0) {
        UInt32 offset = physical % boundaryBytes;

        if (offset + _pageSize > boundaryBytes) {
          continue;
        }
      }

      SetPageUsed(pageIndex);
      ++_usedPages;

      if (zero) {
        UInt8* memory = reinterpret_cast<UInt8*>(pageIndex * _pageSize);

        for (UInt32 b = 0; b < _pageSize; ++b) {
          memory[b] = 0;
        }
      }

      return reinterpret_cast<void*>(physical);
    }

    return nullptr;
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

    if (user) {
      _pageDirectory[pageDirectoryIndex] |= _pageUser;
    }

    CPU::InvalidatePage(virtualAddress);
  }

  UInt32 Memory::GetKernelPageDirectoryPhysical() {
    return KernelVirtualToPhysical(reinterpret_cast<UInt32>(_pageDirectory));
  }

  UInt32 Memory::CreateAddressSpace() {
    UInt32 directoryPhysical = AllocatePhysicalPage(true);

    if (directoryPhysical == 0) {
      return 0;
    }

    UInt32* directory = reinterpret_cast<UInt32*>(directoryPhysical);
    UInt32 kernelStartIndex = kernelVirtualBase >> 22;

    for (UInt32 i = 0; i < _pageDirectoryEntries; ++i) {
      directory[i] = 0;
    }

    for (UInt32 i = kernelStartIndex; i < recursiveSlot; ++i) {
      directory[i] = _pageDirectory[i];
    }

    for (UInt32 i = 0; i < kernelStartIndex; ++i) {
      UInt32 entry = _pageDirectory[i];

      if ((entry & _pagePresent) == 0) {
        continue;
      }

      UInt32 sourceTablePhysical = entry & ~0xFFFu;
      UInt32* sourceTable
        = reinterpret_cast<UInt32*>(sourceTablePhysical);
      UInt32 destTablePhysical = AllocatePhysicalPage(true);

      if (destTablePhysical == 0) {
        PANIC("Failed to allocate page table");
      }

      UInt32* destTable = reinterpret_cast<UInt32*>(destTablePhysical);

      for (UInt32 j = 0; j < _pageTableEntries; ++j) {
        destTable[j] = sourceTable[j];
      }

      directory[i] = (destTablePhysical & ~0xFFFu) | (entry & 0xFFFu);
    }

    directory[recursiveSlot]
      = directoryPhysical | _pagePresent | _pageWrite;

    return directoryPhysical;
  }

  void Memory::DestroyAddressSpace(UInt32 pageDirectoryPhysical) {
    UInt32 kernelDirectory = GetKernelPageDirectoryPhysical();

    if (pageDirectoryPhysical == 0 || pageDirectoryPhysical == kernelDirectory) {
      return;
    }

    UInt32* directory = reinterpret_cast<UInt32*>(pageDirectoryPhysical);
    UInt32 kernelStartIndex = kernelVirtualBase >> 22;

    for (UInt32 i = 0; i < kernelStartIndex; ++i) {
      UInt32 entry = directory[i];

      if ((entry & _pagePresent) == 0) {
        continue;
      }

      UInt32 tablePhysical = entry & ~0xFFFu;
      UInt32* table = reinterpret_cast<UInt32*>(tablePhysical);

      for (UInt32 j = 0; j < _pageTableEntries; ++j) {
        UInt32 page = table[j];

        if ((page & _pagePresent) == 0) {
          continue;
        }

        if ((page & _pageGlobal) != 0) {
          continue;
        }

        UInt32 physical = page & ~0xFFFu;

        if (physical != 0) {
          FreePage(reinterpret_cast<void*>(physical));
        }
      }

      FreePage(reinterpret_cast<void*>(tablePhysical));
    }

    FreePage(reinterpret_cast<void*>(pageDirectoryPhysical));
  }

  void Memory::MapPageInAddressSpace(
    UInt32 pageDirectoryPhysical,
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    if (pageDirectoryPhysical == 0) {
      return;
    }

    UInt32* directory = reinterpret_cast<UInt32*>(pageDirectoryPhysical);
    UInt32 pageDirectoryIndex = (virtualAddress >> 22) & 0x3FF;
    UInt32 pageTableIndex = (virtualAddress >> 12) & 0x3FF;
    UInt32 entry = directory[pageDirectoryIndex];
    UInt32* table = nullptr;

    if ((entry & _pagePresent) != 0) {
      table = reinterpret_cast<UInt32*>(entry & ~0xFFFu);
    } else {
      UInt32 tablePhysical = AllocatePhysicalPage(true);

      if (tablePhysical == 0) {
        PANIC("Failed to allocate page table");
      }

      table = reinterpret_cast<UInt32*>(tablePhysical);
      directory[pageDirectoryIndex]
        = (tablePhysical & ~0xFFFu) | _pagePresent | _pageWrite;
    }

    UInt32 flags = _pagePresent
      | (writable ? _pageWrite : 0)
      | (user ? _pageUser : 0)
      | (global ? _pageGlobal : 0);

    table[pageTableIndex] = (physicalAddress & ~0xFFFu) | flags;

    if (user) {
      directory[pageDirectoryIndex] |= _pageUser;
    }

    if (pageDirectoryPhysical == GetKernelPageDirectoryPhysical()) {
      CPU::InvalidatePage(virtualAddress);
    }
  }

  void Memory::ActivateAddressSpace(UInt32 pageDirectoryPhysical) {
    if (pageDirectoryPhysical == 0) {
      return;
    }

    CPU::LoadPageDirectory(pageDirectoryPhysical);
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

  UInt32 Memory::GetPhysicalAllocatorTotalPages() {
    return _pageCount;
  }

  UInt32 Memory::GetPhysicalAllocatorUsedPages() {
    return _usedPages;
  }

  UInt32 Memory::GetPhysicalAllocatorFreePages() {
    return _pageCount - _usedPages;
  }

  bool Memory::HandlePageFault(
    const Interrupts::Context& context,
    UInt32 faultAddress,
    UInt32 errorCode
  ) {
    CString accessType = (errorCode & 0x2) ? "write" : "read";
    CString mode = (errorCode & 0x4) ? "user" : "kernel";
    bool presentViolation = (errorCode & 0x1) != 0;
    bool reservedBit = (errorCode & 0x8) != 0;
    bool instructionFetch = (errorCode & 0x10) != 0;

    UInt32 pde = GetPageDirectoryEntry(faultAddress);
    UInt32 pte = GetPageTableEntry(faultAddress);

    Logger::Write(LogLevel::Error, ":( PAGE FAULT");
    Logger::WriteFormatted(
      LogLevel::Error,
      "  addr=%p (%s %s) err=%p present=%s reserved=%s instr=%s",
      faultAddress,
      accessType,
      mode,
      errorCode,
      presentViolation ? "yes" : "no",
      reservedBit ? "yes" : "no",
      instructionFetch ? "yes" : "no"
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  EIP=%p ESP=%p CR2=%p PDE=%p PTE=%p",
      context.eip,
      context.esp,
      faultAddress,
      pde,
      pte
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  EAX=%p EBX=%p ECX=%p EDX=%p",
      context.eax,
      context.ebx,
      context.ecx,
      context.edx
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  ESI=%p EDI=%p EBP=%p",
      context.esi,
      context.edi,
      context.ebp
    );
    Logger::WriteFormatted(
      LogLevel::Error,
      "  Task=%u coordinator=%s",
      Kernel::Task::GetCurrentId(),
      Kernel::Task::IsCurrentTaskCoordinator() ? "yes" : "no"
    );
    if ((errorCode & 0x4) != 0) {
      const UInt32* frame = reinterpret_cast<const UInt32*>(&context);
      UInt32 userEsp = frame[13];
      UInt32 userSs = frame[14];

      Logger::WriteFormatted(
        LogLevel::Error,
        "  User ESP=%p SS=%p",
        userEsp,
        userSs
      );
    }

    // stub: escalate for now; future VM/pager can service demand faults
    return false;
  }
}
