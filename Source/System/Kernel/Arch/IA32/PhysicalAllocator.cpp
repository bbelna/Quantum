/**
 * @file System/Kernel/Arch/IA32/PhysicalAllocator.cpp
 * @brief IA32 physical page allocator.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Align.hpp>
#include <Types.hpp>

#include "Arch/IA32/BootInfo.hpp"
#include "Arch/IA32/LinkerSymbols.hpp"
#include "Arch/IA32/Paging.hpp"
#include "Arch/IA32/PhysicalAllocator.hpp"
#include "Logger.hpp"
#include "Panic.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using ::Quantum::AlignDown;
  using ::Quantum::AlignUp;

  using LogLevel = Kernel::Logger::Level;

  UInt32 PhysicalAllocator::KernelVirtualToPhysical(UInt32 virtualAddress) {
    // all kernel segments are offset by kernelVirtualBase
    // compute delta at runtime
    UInt32 kernelPhysicalBase = reinterpret_cast<UInt32>(&__phys_start);
    UInt32 kernelVirtualBase = reinterpret_cast<UInt32>(&__virt_start);

    if (virtualAddress >= Paging::kernelVirtualBase) {
      UInt32 offset = virtualAddress - kernelVirtualBase;

      return kernelPhysicalBase + offset;
    }

    return virtualAddress;
  }

  UInt32 PhysicalAllocator::BitMask(UInt32 bit) {
    return (1u << (bit % 32));
  }

  UInt32 PhysicalAllocator::BitmapWordIndex(UInt32 bit) {
    return bit / 32;
  }

  void PhysicalAllocator::SetPageUsed(UInt32 pageIndex) {
    _pageBitmap[BitmapWordIndex(pageIndex)] |= BitMask(pageIndex);
  }

  void PhysicalAllocator::ClearPageUsed(UInt32 pageIndex) {
    _pageBitmap[BitmapWordIndex(pageIndex)] &= ~BitMask(pageIndex);
  }

  bool PhysicalAllocator::PageFree(UInt32 pageIndex) {
    return
      (_pageBitmap[BitmapWordIndex(pageIndex)]
      & BitMask(pageIndex)) == 0;
  }

  bool PhysicalAllocator::PageUsed(UInt32 pageIndex) {
    return !PageFree(pageIndex);
  }

  int PhysicalAllocator::FindFirstZeroBit(UInt32 value) {
    if (value == 0xFFFFFFFF) return -1;

    for (int i = 0; i < 32; ++i) {
      if ((value & (1u << i)) == 0) return i;
    }

    return -1;
  }

  void PhysicalAllocator::Initialize(UInt32 bootInfoPhysicalAddress) {
    const BootInfo::View* bootInfo = nullptr;
    UInt32 bootInfoPhysical = BootInfo::GetPhysicalAddress();

    if (bootInfoPhysical == 0) {
      bootInfoPhysical = bootInfoPhysicalAddress;
    }

    if (bootInfoPhysical >= pageSize && bootInfoPhysical < _managedBytes) {
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

    _managedBytes = AlignUp(_managedBytes, pageSize);
    _pageCount = _managedBytes / pageSize;

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

        UInt32 startPage = static_cast<UInt32>(baseAddress / pageSize);
        UInt32 endPage
          = static_cast<UInt32>((endAddress + pageSize - 1) / pageSize);

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
    UInt32 usedUntil = AlignUp(bitmapPhysical + bitmapBytes, pageSize);
    UInt32 usedPages = usedUntil / pageSize;

    for (UInt32 i = 0; i < usedPages && i < _pageCount; ++i) {
      SetPageUsed(i);
    }

    UInt32 bootInfoPage = bootInfoPhysical / pageSize;
    UInt32 bootInfoEndPage
      = (bootInfoPhysical + BootInfo::rawSize + pageSize - 1)
      / pageSize;

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
      ReserveRange(
        bootInfo->initBundlePhysical,
        bootInfo->initBundleSize
      );

      UInt32 bundleStart = AlignDown(
        bootInfo->initBundlePhysical,
        pageSize
      );
      UInt32 bundleEnd = AlignUp(
        bootInfo->initBundlePhysical + bootInfo->initBundleSize,
        pageSize
      );

      _initBundleStartPage = bundleStart / pageSize;
      _initBundleEndPage = bundleEnd / pageSize;

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
      = AlignUp(reinterpret_cast<UInt32>(&__phys_end), pageSize);
    UInt32 kernelStartPage = kernelStart / pageSize;
    UInt32 kernelEndPage = kernelEnd / pageSize;

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
    UInt32 stackStartPage = stackBottom / pageSize;
    UInt32 stackEndPage = AlignUp(stackTop, pageSize) / pageSize;

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

  UInt32 PhysicalAllocator::AllocatePage(bool zero) {
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
              "AllocatePage: skipping INIT.BND page %u",
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
          UInt8* memory = reinterpret_cast<UInt8*>(pageIndex * pageSize);

          for (UInt32 b = 0; b < pageSize; ++b) {
            memory[b] = 0;
          }
        }

        return pageIndex * pageSize;
      }
    }

    PANIC("Out of physical memory");

    return 0;
  }

  UInt32 PhysicalAllocator::AllocatePageBelow(
    UInt32 maxPhysicalAddress,
    bool zero,
    UInt32 boundaryBytes
  ) {
    if (maxPhysicalAddress == 0) {
      return 0;
    }

    UInt32 maxPage = maxPhysicalAddress / pageSize;

    if (maxPage > _pageCount) {
      maxPage = _pageCount;
    }

    for (UInt32 pageIndex = 0; pageIndex < maxPage; ++pageIndex) {
      if (!PageFree(pageIndex)) {
        continue;
      }

      UInt32 physical = pageIndex * pageSize;

      if (boundaryBytes != 0) {
        UInt32 offset = physical % boundaryBytes;

        if (offset + pageSize > boundaryBytes) {
          continue;
        }
      }

      SetPageUsed(pageIndex);
      ++_usedPages;

      if (zero) {
        UInt8* memory = reinterpret_cast<UInt8*>(pageIndex * pageSize);

        for (UInt32 b = 0; b < pageSize; ++b) {
          memory[b] = 0;
        }
      }

      return physical;
    }

    return 0;
  }

  void PhysicalAllocator::FreePage(UInt32 physicalAddress) {
    if (physicalAddress % pageSize != 0) {
      Logger::Write(LogLevel::Warning, "FreePage: non-aligned address");
      return;
    }

    UInt32 index = physicalAddress / pageSize;

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

  void PhysicalAllocator::ReserveRange(
    UInt32 physicalAddress,
    UInt32 lengthBytes
  ) {
    UInt32 start = AlignDown(physicalAddress, pageSize);
    UInt32 end = AlignUp(physicalAddress + lengthBytes, pageSize);
    UInt32 startPage = start / pageSize;
    UInt32 endPage = end / pageSize;

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

  void PhysicalAllocator::ReleaseRange(
    UInt32 physicalAddress,
    UInt32 lengthBytes
  ) {
    UInt32 start = AlignDown(physicalAddress, pageSize);
    UInt32 end = AlignUp(physicalAddress + lengthBytes, pageSize);
    UInt32 startPage = start / pageSize;
    UInt32 endPage = end / pageSize;

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

  UInt32 PhysicalAllocator::GetTotalPages() {
    return _pageCount;
  }

  UInt32 PhysicalAllocator::GetUsedPages() {
    return _usedPages;
  }

  UInt32 PhysicalAllocator::GetFreePages() {
    return _pageCount - _usedPages;
  }

  UInt32 PhysicalAllocator::GetManagedBytes() {
    return _managedBytes;
  }
}
