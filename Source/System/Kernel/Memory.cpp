//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Memory.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// Architecture-agnostic memory manager entry points.
//------------------------------------------------------------------------------

#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Types.hpp>

#define MEMORY_DEBUG 0

#define MEMORY_TEST_VERBOSE

#if defined(QUANTUM_ARCH_IA32)
  #include <Arch/IA32/Memory.hpp>
  #include <Arch/IA32/CPU.hpp>

  using ArchMemory = Quantum::Kernel::Arch::IA32::Memory;
  using ArchCPU = Quantum::Kernel::Arch::IA32::CPU;
#else
  #error "No architecture selected for memory manager"
#endif

namespace Quantum::Kernel {
  using LogLevel = Logger::Level;

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
     * Pointer to the end of the mapped heap region (next unmapped byte).
     */
    UInt8* heapMappedEnd = nullptr;

    /**
     * Address of the guard page immediately following the mapped heap.
     */
    UInt8* guardAddress = nullptr;

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
     * Aligns a value down to the nearest alignment boundary.
     * @param value Value to align.
     * @param alignment Alignment boundary (power of two).
     * @return Aligned value at or below input.
     */
    inline UInt32 AlignDown(UInt32 value, UInt32 alignment) {
      return value & ~(alignment - 1);
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
     * Metadata stored immediately before an aligned payload.
     */
    struct AlignedMetadata {
      /**
       * Alignment marker to detect metadata.
       */
      UInt32 magic;

      /**
       * Owning free-block header for the allocation.
       */
      FreeBlock* block;

      /**
       * Offset from the start of the block payload to the aligned address.
       */
      UInt32 payloadOffset;
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
     * Poison pattern used to fill newly allocated payloads.
     */
    constexpr UInt8 poisonAllocated = 0xAA;

    /**
     * Poison pattern used to fill freed payloads.
     */
    constexpr UInt8 poisonFreed = 0x55;

    /**
     * Canary value stored at the end of each allocation.
     */
    constexpr UInt32 canaryValue = 0xDEADC0DE;

    /**
     * Magic tag placed before aligned allocations.
     */
    constexpr UInt32 alignedMagic = 0xA11A0CED;

    /**
     * Maps the next page in the heap virtual range, keeping a guard page
     * unmapped immediately after the mapped region.
     * @return Pointer to the start of the mapped page.
     */
    UInt8* MapNextHeapPage() {
      UInt8* pageStart = heapMappedEnd;
      void* physicalPageAddress = ArchMemory::AllocatePage(true);

      ArchMemory::MapPage(
        reinterpret_cast<UInt32>(heapMappedEnd),
        reinterpret_cast<UInt32>(physicalPageAddress),
        true,
        false,
        false
      );

      heapMappedEnd += heapPageSize;
      heapMappedBytes += heapPageSize;
      guardAddress = heapMappedEnd;

      #if MEMORY_DEBUG
        Logger::WriteFormatted(
          LogLevel::Trace,
          "Heap mapped page at %p (physical %p); mapped bytes now %p",
          pageStart,
          physicalPageAddress,
          heapMappedBytes
        );
      #endif

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
        heapMappedEnd = heapBase;
        heapMappedBytes = 0;
        guardAddress = heapBase;
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
     * Reclaims page-aligned spans inside free blocks back to the physical
     * allocator. Splits blocks as needed and preserves non-page-aligned
     * prefix/suffix fragments.
     */
    void ReclaimPageSpans() {
      FreeBlock* previous = nullptr;
      FreeBlock* current = freeList;

      while (current) {
        UInt8* blockStart = reinterpret_cast<UInt8*>(current);
        UInt8* blockPayload = blockStart + sizeof(FreeBlock);
        UInt8* blockEnd = blockPayload + current->size;
        UInt8* reclaimStart = reinterpret_cast<UInt8*>(
          AlignUp(reinterpret_cast<UInt32>(blockPayload), heapPageSize)
        );
        UInt8* reclaimEnd = reinterpret_cast<UInt8*>(
          AlignDown(reinterpret_cast<UInt32>(blockEnd), heapPageSize)
        );

        if (
          reclaimStart < reclaimEnd &&
          heapMappedBytes > static_cast<UInt32>(reclaimEnd - reclaimStart)
        ) {
          UInt32 reclaimBytes
            = static_cast<UInt32>(reclaimEnd - reclaimStart);
          UInt32 pages = reclaimBytes / heapPageSize;

          // unmap and free each page in the reclaim span
          for (UInt32 i = 0; i < pages; ++i) {
            UInt32 virtualPage
              = reinterpret_cast<UInt32>(reclaimStart) + i * heapPageSize;
            UInt32 pte = ArchMemory::GetPageTableEntry(virtualPage);

            if ((pte & 0x1) == 0) {
              continue;
            }

            UInt32 physical = pte & ~0xFFFu;
            ArchMemory::UnmapPage(virtualPage);

            if (physical) {
              ArchMemory::FreePage(reinterpret_cast<void*>(physical));
            }
            if (heapMappedBytes >= heapPageSize) {
              heapMappedBytes -= heapPageSize;
            }
          }

          // build prefix/suffix fragments if any
          UInt32 prefixBytes
            = static_cast<UInt32>(reclaimStart - blockStart);
          UInt32 suffixBytes
            = static_cast<UInt32>(blockEnd - reclaimEnd);
          FreeBlock* next = current->next;
          FreeBlock* fragmentHead = nullptr;
          FreeBlock* fragmentTail = nullptr;

          auto pushFragment = [&](UInt8* start, UInt32 bytes) {
            if (bytes < sizeof(FreeBlock) + 8) {
              return;
            }

            FreeBlock* frag = reinterpret_cast<FreeBlock*>(start);

            frag->size = bytes - sizeof(FreeBlock);
            frag->next = nullptr;

            if (!fragmentHead) {
              fragmentHead = frag;
              fragmentTail = frag;
            } else {
              fragmentTail->next = frag;
              fragmentTail = frag;
            }
          };

          pushFragment(blockStart, prefixBytes);
          pushFragment(reclaimEnd, suffixBytes);

          // splice fragments into the list in place of the reclaimed block
          if (previous) {
            previous->next = fragmentHead ? fragmentHead : next;
          } else {
            freeList = fragmentHead ? fragmentHead : next;
          }

          if (fragmentTail) {
            fragmentTail->next = next;
            previous = fragmentTail;
          }

          current = next;

          continue;
        }

        previous = current;
        current = current->next;
      }

      CoalesceAdjacentFreeBlocks();
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
      ReclaimPageSpans();
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

  }

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    ArchMemory::InitializePaging(bootInfoPhysicalAddress);

    ArchMemory::PhysicalAllocatorState physicalState = ArchMemory::GetPhysicalAllocatorState();
    UInt64 totalBytes = static_cast<UInt64>(physicalState.totalPages) * heapPageSize;
    UInt64 usedBytes = static_cast<UInt64>(physicalState.usedPages) * heapPageSize;
    UInt64 freeBytes = static_cast<UInt64>(physicalState.freePages) * heapPageSize;

    Logger::WriteFormatted(
      LogLevel::Info,
      "Physical allocator: pages total=%p used=%p free=%p bytes total=%p used=%p free=%p",
      physicalState.totalPages,
      physicalState.usedPages,
      physicalState.freePages,
      totalBytes,
      usedBytes,
      freeBytes
    );
  }

  void* Memory::AllocatePage(bool zero) {
    return ArchMemory::AllocatePage(zero);
  }

  void* Memory::Allocate(Size size) {
    UInt32 requested = AlignUp(static_cast<UInt32>(size), 8);
    int binIndex = BinIndexForSize(requested);
    UInt32 binSize = (binIndex >= 0) ? binSizes[binIndex] : requested;
    UInt32 payloadSize = AlignUp(binSize + sizeof(UInt32), 8); // space for canary
    UInt32 needed = payloadSize + sizeof(FreeBlock);

    EnsureHeapInitialized();

    void* pointer = nullptr;

    while (true) {
      if (binIndex >= 0) {
        pointer = AllocateFromBin(binSize, needed);
      } else {
        pointer = AllocateFromFreeList(needed);
      }

      if (pointer) {
        break;
      }

      UInt8* newPage = MapNextHeapPage();
      FreeBlock* block = reinterpret_cast<FreeBlock*>(newPage);
      block->size = heapPageSize - sizeof(FreeBlock);
      block->next = nullptr;
      InsertFreeBlockSorted(block);
    }

    if (pointer) {
      UInt8* payload = reinterpret_cast<UInt8*>(pointer);
      UInt32 usable = payloadSize - sizeof(UInt32);

      for (UInt32 i = 0; i < usable; ++i) {
        payload[i] = poisonAllocated;
      }

      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);
      *canary = canaryValue;

      #if MEMORY_DEBUG
        Logger::WriteFormatted(
          LogLevel::Trace,
          "Heap alloc ptr=%p block=%p usable=%p size=%p canary=%p mapped=%p",
          payload,
          reinterpret_cast<UInt8*>(payload) - sizeof(FreeBlock),
          usable,
          payloadSize,
          *canary,
          heapMappedBytes
        );
      #endif

      return pointer;
    }

    PANIC("Kernel heap exhausted");

    return nullptr;
  }

  void* Memory::AllocateAligned(Size size, Size alignment) {
    if (alignment <= 8) {
      return Allocate(size);
    }

    if ((alignment & (alignment - 1)) != 0) {
      PANIC("AllocateAligned: alignment must be power of two");
    }

    UInt32 padding = static_cast<UInt32>(alignment) + sizeof(AlignedMetadata);
    void* raw = Allocate(size + padding);

    UInt8* rawBytes = reinterpret_cast<UInt8*>(raw);
    UIntPtr rawAddress = reinterpret_cast<UIntPtr>(rawBytes);
    UIntPtr alignedAddress = (rawAddress + alignment - 1) & ~(alignment - 1);

    AlignedMetadata* metadata
      = reinterpret_cast<AlignedMetadata*>(alignedAddress) - 1;
    metadata->magic = alignedMagic;
    metadata->block = reinterpret_cast<FreeBlock*>(rawBytes - sizeof(FreeBlock));
    metadata->payloadOffset
      = static_cast<UInt32>(alignedAddress - (rawAddress));

    UInt8* alignedPayload = reinterpret_cast<UInt8*>(alignedAddress);
    FreeBlock* block = metadata->block;
    UInt32 usable = block->size - metadata->payloadOffset;

    if (usable < sizeof(UInt32)) {
      PANIC("AllocateAligned: block too small for canary");
    }

    usable -= sizeof(UInt32);

    for (UInt32 i = 0; i < usable; ++i) {
      alignedPayload[i] = poisonAllocated;
    }

    UInt32* canary = reinterpret_cast<UInt32*>(alignedPayload + usable);
    *canary = canaryValue;

    #if MEMORY_DEBUG
      Logger::WriteFormatted(
        LogLevel::Trace,
        "Heap alloc aligned ptr=%p block=%p payload=%p offset=%p usable=%p size=%p canary=%p",
        alignedPayload,
        block,
        reinterpret_cast<UInt8*>(block) + sizeof(FreeBlock),
        metadata->payloadOffset,
        usable,
        block->size,
        *canary
      );
    #endif

    return reinterpret_cast<void*>(alignedAddress);
  }

  void Memory::FreePage(void* page) {
    ArchMemory::FreePage(page);
  }

  void Memory::Free(void* pointer) {
    if (!pointer) return;

    UInt8* bytePointer = reinterpret_cast<UInt8*>(pointer);

    if (
      bytePointer < heapBase ||
      bytePointer >= heapBase + heapMappedBytes
    ) {
      PANIC("Heap free: pointer out of range");
    }

    FreeBlock* block = reinterpret_cast<FreeBlock*>(bytePointer - sizeof(FreeBlock));
    UInt8* blockBytes = reinterpret_cast<UInt8*>(block);
    UInt8* payload = reinterpret_cast<UInt8*>(block) + sizeof(FreeBlock);

    // if the pointer is not at the block payload start, it may be an aligned
    // allocation; verify metadata before using it
    if (bytePointer != payload && bytePointer >= heapBase + sizeof(AlignedMetadata)) {
      AlignedMetadata* metadata
        = reinterpret_cast<AlignedMetadata*>(bytePointer) - 1;

      if (metadata->magic == alignedMagic) {
        UInt8* candidateBlockBytes
          = reinterpret_cast<UInt8*>(metadata->block);

        if (
          candidateBlockBytes >= heapBase &&
          candidateBlockBytes < heapBase + heapMappedBytes
        ) {
          FreeBlock* candidateBlock = metadata->block;
          UInt8* candidatePayload
            = candidateBlockBytes + sizeof(FreeBlock);
          UInt8* candidateAligned
            = candidatePayload + metadata->payloadOffset;
          UInt8* candidateEnd
            = candidatePayload + candidateBlock->size;
          UInt8* metadataBytes = reinterpret_cast<UInt8*>(metadata);

          bool metadataValid
            = metadata->payloadOffset < candidateBlock->size
            && candidateAligned < candidateEnd
            && metadataBytes >= candidatePayload
            && metadataBytes < candidateEnd
            && bytePointer == candidateAligned;

          if (metadataValid) {
            block = candidateBlock;
            blockBytes = candidateBlockBytes;
            payload = candidatePayload;
          }
        }
      }
    }

    if (blockBytes < heapBase || blockBytes >= heapBase + heapMappedBytes) {
      PANIC("Heap free: block pointer invalid");
    }

    // basic sanity: size should not run past mapped heap
    UInt8* blockEnd = payload + block->size;

    if (blockEnd > heapBase + heapMappedBytes) {
      PANIC("Heap free: block overruns mapped region");
    }

    if (block->size < sizeof(UInt32)) {
      PANIC("Heap free: block too small for canary");
    }

    UInt32 offset = 0;

    offset = (bytePointer > payload) ? static_cast<UInt32>(bytePointer - payload) : 0;

    if (offset >= block->size) {
      PANIC("Heap free: offset beyond block size");
    }

    UInt32 usable = block->size - offset;

    if (usable < sizeof(UInt32)) {
      PANIC("Heap free: block too small for canary");
    }

    usable -= sizeof(UInt32);
    UInt8* alignedPayload = payload + offset;
    UInt32* canary = reinterpret_cast<UInt32*>(alignedPayload + usable);

    if (*canary != canaryValue) {
      Logger::WriteFormatted(
        LogLevel::Error,
        "Heap free: canary mismatch ptr=%p block=%p payload=%p offset=%p usable=%p size=%p canary=%p expected=%p",
        bytePointer,
        block,
        payload,
        offset,
        usable,
        block->size,
        *canary,
        canaryValue
      );
      Logger::WriteFormatted(
        LogLevel::Error,
        "Heap state: mapped=%p freeBytes=%p freeBlocks=%p",
        heapMappedBytes,
        GetHeapState().freeBytes,
        GetHeapState().freeBlocks
      );
      PANIC("Heap free: canary corrupted");
    }

    for (UInt32 i = 0; i < usable; ++i) {
      alignedPayload[i] = poisonFreed;
    }

    InsertIntoBinOrFreeList(block);
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
    HeapState state = GetHeapState();

    Logger::WriteFormatted(
      LogLevel::Trace,
      "Heap mapped bytes: %p, free bytes: %p, free blocks: %p",
      state.mappedBytes,
      state.freeBytes,
      state.freeBlocks
    );
  }

  void Memory::Test() {
    Logger::Write(LogLevel::Trace, "Performing memory subsystem test");

    HeapState before = GetHeapState();

    void* a = Allocate(32);
    void* b = Allocate(64);

    if (!a || !b) {
      PANIC("Allocation returned null");
    }

    // write/read patterns to ensure writable pages
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
      Logger::WriteFormatted(
        LogLevel::Trace,
        "Memory state before self-test: %p mapped, %p free, %p blocks",
        before.mappedBytes,
        before.freeBytes,
        before.freeBlocks
      );
      Logger::WriteFormatted(
        LogLevel::Trace,
        "Memory state after self-test: %p mapped, %p free, %p blocks",
        after.mappedBytes,
        after.freeBytes,
        after.freeBlocks
      );
    #endif

    Logger::Write(LogLevel::Trace, "Memory self-test passed");
  }

  bool Memory::VerifyHeap() {
    EnsureHeapInitialized();

    // verify free list ordering and bounds
    FreeBlock* current = freeList;
    FreeBlock* last = nullptr;

    while (current) {
      UInt8* blockBytes = reinterpret_cast<UInt8*>(current);
      UInt8* payload = blockBytes + sizeof(FreeBlock);
      UInt8* blockEnd = payload + current->size;

      if (blockBytes < heapBase || blockEnd > heapBase + heapMappedBytes) {
        PANIC("VerifyHeap: free block out of bounds");
      }

      if (last && current <= last) {
        PANIC("VerifyHeap: free list not strictly increasing");
      }

      last = current;
      current = current->next;
    }

    // verify canaries of all free blocks (payload poisoned but canary intact)
    current = freeList;

    while (current) {
      UInt8* payload = reinterpret_cast<UInt8*>(current) + sizeof(FreeBlock);

      if (current->size < sizeof(UInt32)) {
        PANIC("VerifyHeap: free block too small for canary");
      }

      UInt32 usable = current->size - sizeof(UInt32);
      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

      if (*canary != canaryValue) {
        PANIC("VerifyHeap: free block canary corrupted");
      }

      current = current->next;
    }

    return true;
  }

  void Memory::CheckHeap() {
    bool ok = VerifyHeap();
    Logger::WriteFormatted(
      ok ? LogLevel::Info : LogLevel::Error,
      "Heap verify %s",
      ok ? "ok" : "failed"
    );
  }
}
