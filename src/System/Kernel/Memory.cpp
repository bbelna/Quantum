//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Memory.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Architecture-agnostic memory manager entry points.
//------------------------------------------------------------------------------

#include <Drivers/Console.hpp>
#include <Kernel.hpp>
#include <KernelTypes.hpp>
#include <Memory.hpp>

#define MEMORY_TEST_VERBOSE

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Memory.hpp>
  #include <Arch/IA32/CPU.hpp>

  namespace ArchMemory = Quantum::Kernel::Arch::IA32;
  using ArchCPU = Quantum::Kernel::Arch::IA32::CPU;
#else
  #error "No architecture selected for memory manager"
#endif

namespace Quantum::Kernel {
  namespace {
    /**
     * Heap page size.
     */
    constexpr UInt32 heapPageSize  = 4096;

    /**
     * Heap start virtual address.
     */
    constexpr UInt32 heapStartVirtualAddress = 0x00400000;

    /**
     * Number of guard pages before the heap.
     */
    constexpr UInt32 heapGuardPagesBefore = 1;

    /**
     * Number of guard pages after the heap.
     */
    constexpr UInt32 heapGuardPagesAfter  = 1;

    /**
     * Pointer to the start of the heap region.
     */
    UInt8* heapBase = nullptr;

    /**
     * Pointer to the end of the heap region (next unmapped byte).
     */
    UInt8* heapEnd = nullptr;

    /**
     * Pointer to the current guard page (if any).
     */
    UInt8* guardPage = nullptr;

    /**
     * Number of bytes currently mapped in the heap.
     */
    UInt32 heapMappedBytes = 0;

    /**
     * Pointer to the current position in the heap for allocations.
     */
    UInt8* heapCurrent = nullptr;

    /**
     * Aligns a value to the next multiple of alignment.
     * @param value Value to align.
     * @param alignment Alignment boundary (power of two).
     * @return Aligned value.
     */
    inline UInt32 AlignUp(UInt32 value, UInt32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }

    /**
     * Header for each heap allocation or free block.
     */
    struct FreeBlock {
      /**
       * Bytes in block payload.
       */
      UInt32 size;

      /**
       * Next free block in list.
       */
      FreeBlock* next;
    };

    /**
     * Head of the general free list.
     */
    FreeBlock* freeList = nullptr;

    /**
     * Number of fixed-size bins.
     */
    constexpr UInt32 binCount = 4;

    /**
     * Sizes of fixed-size bins.
     */
    constexpr UInt32 binSizes[binCount] = { 16, 32, 64, 128 };

    /**
     * Free lists for each fixed-size bin.
     */
    FreeBlock* binFreeLists[binCount] = { nullptr, nullptr, nullptr, nullptr };

    /**
     * Maps the next page in the heap virtual range and advances heapEnd.
     * @return Pointer to the start of the mapped page.
     */
    UInt8* MapNextHeapPage() {
      UInt8* pageStart = heapEnd;
      void* physicalPageAddress = ArchMemory::AllocatePage();

      ArchMemory::MapPage(
        reinterpret_cast<UInt32>(heapEnd),
        reinterpret_cast<UInt32>(physicalPageAddress),
        true
      );

      heapEnd += heapPageSize;
      heapMappedBytes += heapPageSize;

      return pageStart;
    }

    /**
     * Lazily initializes heap bookkeeping on first use.
     */
    void EnsureHeapInitialized() {
      if (!heapBase) {
        heapBase = reinterpret_cast<UInt8*>(
          heapStartVirtualAddress + heapGuardPagesBefore * heapPageSize
        );
        heapCurrent = heapBase;
        heapEnd = heapBase;
        heapMappedBytes = 0;
        guardPage = MapNextHeapPage(); // reserve first mapped page as guard
        freeList = nullptr;
      }
    }

    /**
     * Merges adjacent free blocks to reduce fragmentation.
     */
    void CoalesceAdjacentFreeBlocks() {
      FreeBlock* current = freeList;

      while (current && current->next) {
        UInt8* currentEnd
          = reinterpret_cast<UInt8*>(current)
          + sizeof(FreeBlock)
          + current->size;

        if (currentEnd == reinterpret_cast<UInt8*>(current->next)) {
          current->size += sizeof(FreeBlock) + current->next->size;
          current->next = current->next->next;
        } else {
          current = current->next;
        }
      }
    }

    /**
     * Inserts a free block into the sorted free list and coalesces neighbors.
     * @param block Block to insert.
     */
    void InsertFreeBlockSorted(FreeBlock* block) {
      if (!freeList || block < freeList) {
        block->next = freeList;
        freeList = block;
      } else {
        FreeBlock* current = freeList;

        while (current->next && current->next < block) {
          current = current->next;
        }

        block->next = current->next;
        current->next = block;
      }

      CoalesceAdjacentFreeBlocks();
    }

    /**
     * Attempts to satisfy an allocation from the general free list.
     * @param needed Total bytes requested including header.
     * @return Pointer to payload or `nullptr` if none fit.
     */
    void* AllocateFromFreeList(UInt32 needed) {
      FreeBlock* previous = nullptr;
      FreeBlock* current = freeList;

      while (current) {
        // sanity: block must fit within mapped heap
        UInt8* blockStart = reinterpret_cast<UInt8*>(current);
        UInt8* blockEnd = blockStart + sizeof(FreeBlock) + current->size;

        if (blockStart < heapBase || blockEnd > heapBase + heapMappedBytes) {
          PANIC("Heap corruption detected");
        }

        UInt32 total = current->size + sizeof(FreeBlock);
        if (total >= needed) {
          // split if enough space remains for another block
          if (total >= needed + sizeof(FreeBlock) + 8) {
            UInt8* newBlockAddr = reinterpret_cast<UInt8*>(current) + needed;
            FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(newBlockAddr);
            newBlock->size = total - needed - sizeof(FreeBlock);
            newBlock->next = current->next;
            current->size = needed - sizeof(FreeBlock);
            current->next = nullptr;

            if (previous) {
              previous->next = newBlock;
            } else {
              freeList = newBlock;
            }
          } else {
            // remove entire block
            if (previous) {
              previous->next = current->next;
            } else {
              freeList = current->next;
            }
          }

          return reinterpret_cast<UInt8*>(current) + sizeof(FreeBlock);
        }

        previous = current;
        current = current->next;
      }

      return nullptr;
    }

    /**
     * Determines the bin index for a requested payload size.
     * @param size Payload bytes requested.
     * @return Bin index or -1 if it does not fit in a fixed bin.
     */
    int BinIndexForSize(UInt32 size) {
      for (UInt32 i = 0; i < binCount; ++i) {
        if (size <= binSizes[i]) {
          return static_cast<int>(i);
        }
      }

      return -1;
    }

    /**
     * Allocates from a fixed-size bin if available, otherwise falls back to free list.
     * @param binSize Bin payload size to request.
     * @param neededWithHeader Total bytes including header.
     * @return Pointer to payload or nullptr.
     */
    void* AllocateFromBin(UInt32 binSize, UInt32 neededWithHeader) {
      int index = BinIndexForSize(binSize);

      if (index < 0) {
        return nullptr;
      } else {
        if (binFreeLists[index]) {
          FreeBlock* block = binFreeLists[index];
          binFreeLists[index] = block->next;

          return reinterpret_cast<UInt8*>(block) + sizeof(FreeBlock);
        }

        // fallback to general free list
        void* pointer = AllocateFromFreeList(neededWithHeader);

        return pointer;
      }
    }

    /**
     * Returns a freed block either to a size bin or the general free list.
     * @param block Block being freed.
     */
    void InsertIntoBinOrFreeList(FreeBlock* block) {
      int index = BinIndexForSize(block->size);

      if (index >= 0) {
        block->next = binFreeLists[index];
        binFreeLists[index] = block;
      } else {
        InsertFreeBlockSorted(block);
      }
    }

    /**
     * Ensures the heap has room for an allocation of the given size.
     * @param size Bytes required (including header).
     */
    void EnsureHeapHasSpace(UInt32 size) {
      EnsureHeapInitialized();

      while (true) {
        void* pointer = AllocateFromFreeList(size);

        if (pointer) {
          Memory::Free(pointer); // availability confirmed
          break;
        }

        // map a new page: release previous guard (if any) into free list, set
        // new guard
        if (guardPage) {
          FreeBlock* block = reinterpret_cast<FreeBlock*>(guardPage);
          block->size = heapPageSize - sizeof(FreeBlock);
          block->next = nullptr;

          InsertFreeBlockSorted(block);
        }

        guardPage = MapNextHeapPage();
      }
    }
  }

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    ArchMemory::InitializePaging(bootInfoPhysicalAddress);
  }

  void* Memory::AllocatePage() {
    return ArchMemory::AllocatePage();
  }

  void* Memory::Allocate(Size size) {
    UInt32 requested = AlignUp(static_cast<UInt32>(size), 8);
    int binIndex = BinIndexForSize(requested);
    UInt32 binSize = (binIndex >= 0) ? binSizes[binIndex] : requested;
    UInt32 needed = AlignUp(binSize, 8) + sizeof(FreeBlock);

    EnsureHeapHasSpace(needed);

    void* pointer = (binIndex >= 0)
      ? AllocateFromBin(binSize, needed)
      : AllocateFromFreeList(needed);

    if (pointer) {
      return pointer;
    }

    PANIC("Kernel heap exhausted");

    return nullptr;
  }

  void Memory::FreePage(void* page) {
    ArchMemory::FreePage(page);
  }

  void Memory::Free(void* pointer) {
    if (!pointer) return;

    UInt8* bytePointer = reinterpret_cast<UInt8*>(pointer);

    if (
      bytePointer < heapBase + sizeof(FreeBlock) ||
      bytePointer >= heapBase + heapMappedBytes
    ) {
      PANIC("Heap free: pointer out of range");
    } else {
      FreeBlock* block = reinterpret_cast<FreeBlock*>(
        bytePointer - sizeof(FreeBlock)
      );

      // basic sanity: size should not run past mapped heap
      UInt8* blockEnd
        = reinterpret_cast<UInt8*>(block)
        + sizeof(FreeBlock)
        + block->size;

      if (blockEnd > heapBase + heapMappedBytes) {
        PANIC("Heap free: block overruns mapped region");
      } else {
        InsertIntoBinOrFreeList(block);
      }
    }
  }

  Memory::HeapState Memory::GetHeapState() {
    HeapState state{};

    state.mappedBytes = heapMappedBytes;

    UInt32 freeBytes = 0;
    UInt32 blocks = 0;
    FreeBlock* current = freeList;

    while (current) {
      freeBytes += current->size;
      ++blocks;
      current = current->next;
    }

    state.freeBytes = freeBytes;
    state.freeBlocks = blocks;

    return state;
  }

  void Memory::DumpState() {
    using Drivers::Console;

    HeapState state = GetHeapState();

    Console::Write("Heap mapped bytes: ");
    Console::WriteHex32(state.mappedBytes);
    Console::Write(", free bytes: ");
    Console::WriteHex32(state.freeBytes);
    Console::Write(", free blocks: ");
    Console::WriteHex32(state.freeBlocks);
    Console::WriteLine("");
  }

  void Memory::Test() {
    using Drivers::Console;

    Console::WriteLine("Performing memory subsystem test");

    HeapState before = GetHeapState();

    void* a = Allocate(32);
    void* b = Allocate(64);

    if (!a || !b) {
      PANIC("Allocation returned null");
    }

    // Write/read patterns to ensure writable pages.
    UInt8* pa = reinterpret_cast<UInt8*>(a);
    UInt8* pb = reinterpret_cast<UInt8*>(b);

    for (Size i = 0; i < 32; ++i) {
      pa[i] = static_cast<UInt8>(i);
      if (pa[i] != static_cast<UInt8>(i)) {
        PANIC("Heap write/read mismatch");
      }
    }

    for (Size i = 0; i < 64; ++i) {
      pb[i] = static_cast<UInt8>(0xA5);
      if (pb[i] != static_cast<UInt8>(0xA5)) {
        PANIC("Heap write/read mismatch");
      }
    }

    Free(b);
    Free(a);

    HeapState after = GetHeapState();

    if (after.freeBytes < before.freeBytes) {
      PANIC("Free bytes decreased unexpectedly");
    }

    #ifdef MEMORY_TEST_VERBOSE
      Console::WriteLine("Memory state before self-test:");
      Console::Write("  ");
      Console::WriteHex32(before.mappedBytes);
      Console::Write(" mapped, ");
      Console::WriteHex32(before.freeBytes);
      Console::Write(" free, ");
      Console::WriteHex32(before.freeBlocks);
      Console::WriteLine(" blocks");

      Console::WriteLine("Memory state after self-test:");
      Console::Write("  ");
      Console::WriteHex32(after.mappedBytes);
      Console::Write(" mapped, ");
      Console::WriteHex32(after.freeBytes);
      Console::Write(" free, ");
      Console::WriteHex32(after.freeBlocks);
      Console::WriteLine(" blocks");
    #endif

    Console::WriteLine("Memory self-test passed");
  }
}
