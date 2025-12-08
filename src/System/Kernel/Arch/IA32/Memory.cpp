//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Memory.cpp
// Brandon Belna - MIT License
//------------------------------------------------------------------------------
// IA32 paging setup and simple physical page allocator.
//------------------------------------------------------------------------------

#include <Arch/IA32/Memory.hpp>
#include <Arch/IA32/CPU.hpp>
#include <KernelTypes.hpp>
#include <Drivers/Console.hpp>

extern "C" uint8 __bss_end;

namespace Quantum::Kernel::Arch::IA32 {
  namespace {
    constexpr uint32 pageSize    = 4096;
    constexpr uint32 pagePresent = 0x1;
    constexpr uint32 pageWrite   = 0x2;

    // For now, manage only the first 4 MB (1024 pages) to keep identity-mapped.
    constexpr uint32 managedBytes = 4 * 1024 * 1024;
    constexpr uint32 pageCount    = managedBytes / pageSize; // 1024 pages

    // 4 KB aligned page directory and first page table.
    static uint32 pageDirectory[1024] __attribute__((aligned(pageSize)));
    static uint32 firstPageTable[1024] __attribute__((aligned(pageSize)));

    // Physical page bitmap (1 bit per page), placed after BSS.
    static uint32* pageBitmap = nullptr;
    static uint32  bitmapLengthWords = 0;

    inline uint32 AlignUp(uint32 value, uint32 align) {
      return (value + align - 1) & ~(align - 1);
    }

    inline void SetPageUsed(uint32 pageIndex) {
      pageBitmap[pageIndex / 32] |= (1u << (pageIndex % 32));
    }

    inline bool PageFree(uint32 pageIndex) {
      return (pageBitmap[pageIndex / 32] & (1u << (pageIndex % 32))) == 0;
    }

    void InitPhysicalAllocator() {
      uint32 bitmapBytes = AlignUp((pageCount + 7) / 8, 4);
      uint32 bitmapPhys  = AlignUp(reinterpret_cast<uint32>(&__bss_end), 4);
      pageBitmap         = reinterpret_cast<uint32*>(bitmapPhys);
      bitmapLengthWords  = bitmapBytes / 4;

      // Clear bitmap
      for (uint32 i = 0; i < bitmapLengthWords; ++i) {
        pageBitmap[i] = 0;
      }

      // Mark pages occupied by the bitmap and anything below it (kernel, bss, etc.)
      uint32 usedUntil = AlignUp(bitmapPhys + bitmapBytes, pageSize);
      uint32 usedPages = usedUntil / pageSize;
      for (uint32 i = 0; i < usedPages && i < pageCount; ++i) {
        SetPageUsed(i);
      }

      // Mark the page directory and first table physical frames.
      SetPageUsed(reinterpret_cast<uint32>(pageDirectory) / pageSize);
      SetPageUsed(reinterpret_cast<uint32>(firstPageTable) / pageSize);
    }

    uint32 AllocatePhysicalPage() {
      for (uint32 i = 0; i < pageCount; ++i) {
        if (PageFree(i)) {
          SetPageUsed(i);
          return i * pageSize;
        }
      }

      Quantum::Kernel::Drivers::Console::WriteLine("Out of physical memory");
      for (;;) { asm volatile("hlt"); }
    }
  }

  void InitializePaging() {
    InitPhysicalAllocator();

    // Clear directory and table.
    for (int i = 0; i < 1024; ++i) {
      pageDirectory[i] = 0;
      firstPageTable[i] = 0;
    }

    // Identity map the first 4 MB.
    for (uint32 i = 0; i < 1024; ++i) {
      firstPageTable[i] = (i * pageSize) | pagePresent | pageWrite;
    }
    pageDirectory[0] = reinterpret_cast<uint32>(firstPageTable) | pagePresent | pageWrite;

    // Load directory and enable paging.
    CPU::LoadPageDirectory(reinterpret_cast<uint32>(pageDirectory));
    CPU::EnablePaging();
  }

  void* AllocatePage() {
    uint32 phys = AllocatePhysicalPage();
    // Identity mapped region for now.
    return reinterpret_cast<void*>(phys);
  }

  void MapPage(uint32 virtualAddr, uint32 physicalAddr, bool writable) {
    uint32 pdIndex = (virtualAddr >> 22) & 0x3FF;
    uint32 ptIndex = (virtualAddr >> 12) & 0x3FF;

    uint32* table;
    if (pageDirectory[pdIndex] & pagePresent) {
      table = reinterpret_cast<uint32*>(pageDirectory[pdIndex] & ~0xFFF);
    } else {
      // Allocate a new page table in low memory (identity mapped).
      table = reinterpret_cast<uint32*>(AllocatePhysicalPage());
      for (int i = 0; i < 1024; ++i) {
        table[i] = 0;
      }
      pageDirectory[pdIndex] = reinterpret_cast<uint32>(table) | pagePresent | pageWrite;
    }

    uint32 flags = pagePresent | (writable ? pageWrite : 0);
    table[ptIndex] = (physicalAddr & ~0xFFF) | flags;
    CPU::InvalidatePage(virtualAddr);
  }
}
