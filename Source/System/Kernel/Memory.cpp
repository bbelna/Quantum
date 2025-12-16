/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Memory.cpp
 * Architecture-agnostic memory manager.
 */

#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Types/Primitives.hpp>

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
    constexpr UInt32 _heapPageSize  = 4096;

    /**
     * Heap start virtual address.
     */
    constexpr UInt32 _heapStartVirtualAddress = 0x00400000;

    /**
     * Number of guard pages before the heap.
     */
    constexpr UInt32 _heapGuardPagesBefore = 1;

    /**
     * Number of guard pages after the heap.
     */
    constexpr UInt32 _heapGuardPagesAfter  = 1;

    /**
     * Pointer to the start of the heap region.
     */
    UInt8* _heapBase = nullptr;

    /**
     * Pointer to the end of the mapped heap region (next unmapped byte).
     */
    UInt8* _heapMappedEnd = nullptr;

    /**
     * Address of the guard page immediately following the mapped heap.
     */
    UInt8* _guardAddress = nullptr;

    /**
     * Number of bytes currently mapped in the heap.
     */
    UInt32 _heapMappedBytes = 0;

    /**
     * Pointer to the current position in the heap for allocations.
     */
    UInt8* _heapCurrent = nullptr;

    /**
     * Tracks the minimum contiguous pages we should keep free at the tail of
     * the heap to satisfy the largest allocation request seen so far.
     */
    UInt32 _requiredTailPages = 2;

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
     * Writes the canary for a free block at the end of its payload.
     */
    inline void SetFreeBlockCanary(FreeBlock* block) {
      if (block->size < sizeof(UInt32)) {
        PANIC("Free block too small for canary");
      }

      UInt8* payload = reinterpret_cast<UInt8*>(block) + sizeof(FreeBlock);
      UInt32 usable = block->size - sizeof(UInt32);
      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);
      *canary = canaryValue;
    }

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
      UInt8* pageStart = _heapMappedEnd;
      void* physicalPageAddress = ArchMemory::AllocatePage(true);

      ArchMemory::MapPage(
        reinterpret_cast<UInt32>(_heapMappedEnd),
        reinterpret_cast<UInt32>(physicalPageAddress),
        true,
        false,
        false
      );

      _heapMappedEnd += _heapPageSize;
      _heapMappedBytes += _heapPageSize;
      _guardAddress = _heapMappedEnd;

      Logger::WriteFormatted(
        LogLevel::Debug,
        "Heap mapped page at %p (physical %p); mapped bytes now %p",
        pageStart,
        physicalPageAddress,
        _heapMappedBytes
      );

      return pageStart;
    }

    /**
     * Lazily initializes heap bookkeeping on first use.
     */
    void EnsureHeapInitialized() {
      if (!_heapBase) {
        _heapBase = reinterpret_cast<UInt8*>(
          _heapStartVirtualAddress + _heapGuardPagesBefore * _heapPageSize
        );
        _heapCurrent = _heapBase;
        _heapMappedEnd = _heapBase;
        _heapMappedBytes = 0;
        _guardAddress = _heapBase;
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

      // refresh canaries for all free blocks after any coalescing
      current = freeList;

      while (current) {
        SetFreeBlockCanary(current);
        current = current->next;
      }
    }

    /**
     * Reclaims pages only from the end of the heap so the mapped region
     * remains contiguous. This avoids holes that would invalidate the
     * heapMappedBytes "end" invariant used by the allocator sanity checks.
     */
    void ReclaimPageSpans() {
      if (!freeList) {
        return;
      }

      // find the highest-addressed free block (end of heap)
      FreeBlock* previous = nullptr;
      FreeBlock* current = freeList;

      while (current->next) {
        previous = current;
        current = current->next;
      }

      UInt8* blockStart = reinterpret_cast<UInt8*>(current);
      UInt8* blockPayload = blockStart + sizeof(FreeBlock);
      UInt8* blockEnd = blockPayload + current->size;
      UInt8* heapEnd = _heapBase + _heapMappedBytes;

      // only reclaim if this block reaches the mapped end of the heap
      if (blockEnd != heapEnd) {
        return;
      }

      UInt8* reclaimStart = reinterpret_cast<UInt8*>(
        AlignUp(reinterpret_cast<UInt32>(blockPayload), _heapPageSize)
      );

      if (reclaimStart >= heapEnd) {
        return;
      }

      UInt32 reclaimablePages
        = static_cast<UInt32>((heapEnd - reclaimStart) / _heapPageSize);

      UInt32 reserveTailPages = _requiredTailPages;
      if (reserveTailPages < 2) {
        reserveTailPages = 2;
      }

      if (reclaimablePages <= reserveTailPages) {
        return;
      }

      UInt32 pagesToReclaim = reclaimablePages - reserveTailPages;

      for (UInt32 i = 0; i < pagesToReclaim; ++i) {
        UInt32 virtualPage
          = reinterpret_cast<UInt32>(reclaimStart) + i * _heapPageSize;
        UInt32 pte = ArchMemory::GetPageTableEntry(virtualPage);

        if ((pte & 0x1) == 0) {
          continue;
        }

        UInt32 physical = pte & ~0xFFFu;
        ArchMemory::UnmapPage(virtualPage);

        if (physical) {
          ArchMemory::FreePage(reinterpret_cast<void*>(physical));
        }

        if (_heapMappedBytes >= _heapPageSize) {
          _heapMappedBytes -= _heapPageSize;
        }

        if (_heapMappedEnd >= _heapBase + _heapPageSize) {
          _heapMappedEnd -= _heapPageSize;
        }
      }

      _guardAddress = _heapMappedEnd;

      // shrink the tail block to the remaining bytes before the reclaimed span
      UInt8* newHeapEnd = _heapBase + _heapMappedBytes;
      UInt32 newSize = static_cast<UInt32>(newHeapEnd - blockPayload);

      if (newSize < sizeof(UInt32)) {
        // drop the block if too small to hold a canary
        if (previous) {
          previous->next = nullptr;
        } else {
          freeList = nullptr;
        }
      } else {
        current->size = newSize;
        current->next = nullptr;
        SetFreeBlockCanary(current);
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

        if (blockStart < _heapBase || blockEnd > _heapBase + _heapMappedBytes) {
          Logger::WriteFormatted(
            LogLevel::Error,
            "AllocateFromFreeList: corrupt block addr=%p size=%p end=%p (heap base=%p end=%p needed=%p)",
            blockStart,
            current->size,
            blockEnd,
            _heapBase,
            _heapBase + _heapMappedBytes,
            needed
          );
          
          // dump entire free list
          Logger::Write(LogLevel::Error, "Free list dump:");
          FreeBlock* dump = freeList;
          int count = 0;
          
          while (dump && count < 20) {
            UInt8* dumpStart = reinterpret_cast<UInt8*>(dump);
            UInt8* dumpEnd = dumpStart + sizeof(FreeBlock) + dump->size;
            
            Logger::WriteFormatted(
              LogLevel::Error,
              "  Block %d: addr=%p size=%p end=%p next=%p",
              count,
              dumpStart,
              dump->size,
              dumpEnd,
              dump->next
            );
            
            count++;
            dump = dump->next;
          }
          
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
            SetFreeBlockCanary(newBlock);
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
        SetFreeBlockCanary(block);
      } else {
        InsertFreeBlockSorted(block);
      }
    }

  }

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    ArchMemory::InitializePaging(bootInfoPhysicalAddress);

    ArchMemory::PhysicalAllocatorState physicalState = ArchMemory::GetPhysicalAllocatorState();
    UInt64 totalBytes = static_cast<UInt64>(physicalState.TotalPages) * _heapPageSize;
    UInt64 usedBytes = static_cast<UInt64>(physicalState.UsedPages) * _heapPageSize;
    UInt64 freeBytes = static_cast<UInt64>(physicalState.FreePages) * _heapPageSize;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Physical allocator: pages total=%p used=%p free=%p bytes total=%p used=%p free=%p",
      physicalState.TotalPages,
      physicalState.UsedPages,
      physicalState.FreePages,
      totalBytes,
      usedBytes,
      freeBytes
    );
    DumpState();

    #ifdef MEMORY_TEST
      Memory::Test();
      Memory::ResetHeap();
      Memory::VerifyHeap();
    #endif
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
    UInt32 pagesNeeded = (needed + _heapPageSize - 1) / _heapPageSize;
    if (pagesNeeded > _requiredTailPages) {
      _requiredTailPages = pagesNeeded;
    }

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Allocate: requested=%p binIndex=%d binSize=%p payloadSize=%p needed=%p",
      requested,
      binIndex,
      binSize,
      payloadSize,
      needed
    );

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

      // Map enough contiguous pages to satisfy this allocation in a single block.
      UInt32 pagesToMap = (needed + _heapPageSize - 1) / _heapPageSize;
      if (pagesToMap == 0) {
        pagesToMap = 1;
      }

      UInt8* firstPage = MapNextHeapPage();
      for (UInt32 i = 1; i < pagesToMap; ++i) {
        MapNextHeapPage();
      }

      UInt32 totalBytes = pagesToMap * _heapPageSize;
      FreeBlock* block = reinterpret_cast<FreeBlock*>(firstPage);
      block->size = totalBytes - sizeof(FreeBlock);
      block->next = nullptr;

      SetFreeBlockCanary(block);
      InsertFreeBlockSorted(block);
    }

    if (pointer) {
      UInt8* payload = reinterpret_cast<UInt8*>(pointer);
      FreeBlock* blk = reinterpret_cast<FreeBlock*>(payload - sizeof(FreeBlock));

      if (blk->size < sizeof(UInt32)) {
        PANIC("Heap alloc: block too small for canary");
      }

      UInt32 usable = blk->size - sizeof(UInt32);

      for (UInt32 i = 0; i < usable; ++i) {
        payload[i] = poisonAllocated;
      }

      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);
      *canary = canaryValue;

      Logger::WriteFormatted(
        LogLevel::Debug,
        "Heap alloc ptr=%p block=%p usable=%p size=%p canary=%p mapped=%p",
        payload,
        reinterpret_cast<UInt8*>(payload) - sizeof(FreeBlock),
        usable,
        payloadSize,
        *canary,
        _heapMappedBytes
      );

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

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Heap alloc aligned ptr=%p block=%p payload=%p offset=%p usable=%p size=%p canary=%p",
      alignedPayload,
      block,
      reinterpret_cast<UInt8*>(block) + sizeof(FreeBlock),
      metadata->payloadOffset,
      usable,
      block->size,
      *canary
    );

    return reinterpret_cast<void*>(alignedAddress);
  }

  void Memory::FreePage(void* page) {
    ArchMemory::FreePage(page);
  }

  void Memory::Free(void* pointer) {
    if (!pointer) return;

    UInt8* bytePointer = reinterpret_cast<UInt8*>(pointer);

    if (
      bytePointer < _heapBase ||
      bytePointer >= _heapBase + _heapMappedBytes
    ) {
      PANIC("Heap free: pointer out of range");
    }

    FreeBlock* block = reinterpret_cast<FreeBlock*>(bytePointer - sizeof(FreeBlock));
    UInt8* blockBytes = reinterpret_cast<UInt8*>(block);
    UInt8* payload = reinterpret_cast<UInt8*>(block) + sizeof(FreeBlock);

    // if the pointer is not at the block payload start, it may be an aligned
    // allocation; verify metadata before using it
    if (bytePointer != payload && bytePointer >= _heapBase + sizeof(AlignedMetadata)) {
      AlignedMetadata* metadata
        = reinterpret_cast<AlignedMetadata*>(bytePointer) - 1;

      if (metadata->magic == alignedMagic) {
        UInt8* candidateBlockBytes
          = reinterpret_cast<UInt8*>(metadata->block);

        if (
          candidateBlockBytes >= _heapBase &&
          candidateBlockBytes < _heapBase + _heapMappedBytes
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

    if (blockBytes < _heapBase || blockBytes >= _heapBase + _heapMappedBytes) {
      PANIC("Heap free: block pointer invalid");
    }

    // basic sanity: size should not run past mapped heap
    UInt8* blockEnd = payload + block->size;

    if (blockEnd > _heapBase + _heapMappedBytes) {
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
        _heapMappedBytes,
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

    state.mappedBytes = _heapMappedBytes;

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
      LogLevel::Debug,
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

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Memory state before self-test: %p mapped, %p free, %p blocks",
      before.mappedBytes,
      before.freeBytes,
      before.freeBlocks
    );
    Logger::WriteFormatted(
      LogLevel::Debug,
      "Memory state after self-test: %p mapped, %p free, %p blocks",
      after.mappedBytes,
      after.freeBytes,
      after.freeBlocks
    );

    Logger::Write(LogLevel::Trace, "Memory self-test passed");
  }

  bool Memory::VerifyHeap() {
    EnsureHeapInitialized();

    bool ok = true;

    // verify free list ordering and bounds
    FreeBlock* current = freeList;
    FreeBlock* last = nullptr;

    while (current) {
      UInt8* blockBytes = reinterpret_cast<UInt8*>(current);
      UInt8* payload = blockBytes + sizeof(FreeBlock);
      UInt8* blockEnd = payload + current->size;

      if (blockBytes < _heapBase || blockEnd > _heapBase + _heapMappedBytes) {
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
        Logger::Write(LogLevel::Error, "VerifyHeap: free block too small for canary");
        ok = false;
        break;
      }

      UInt32 usable = current->size - sizeof(UInt32);
      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

      if (*canary != canaryValue) {
        Logger::Write(LogLevel::Error, "VerifyHeap: free block canary corrupted");
        ok = false;
        break;
      }

      current = current->next;
    }

    // dump free list snapshot for debugging
    Logger::Write(LogLevel::Debug, "Free list dump:");
    current = freeList;
    int count = 0;

    while (current && count < 20) {
      UInt8* blockStart = reinterpret_cast<UInt8*>(current);
      UInt8* blockEnd = blockStart + sizeof(FreeBlock) + current->size;

      Logger::WriteFormatted(
        LogLevel::Debug,
        "  Block %d: addr=%p size=%p end=%p (heap base=%p end=%p)",
        count,
        blockStart,
        current->size,
        blockEnd,
        _heapBase,
        _heapBase + _heapMappedBytes
      );

      count++;
      current = current->next;
    }

    Logger::WriteFormatted(
      ok ? LogLevel::Debug : LogLevel::Error,
      "Heap verify %s",
      ok ? "ok" : "failed"
    );

    return ok;
  }

  void Memory::ResetHeap() {
    EnsureHeapInitialized();

    // clear bin free lists
    for (UInt32 i = 0; i < binCount; ++i) {
      binFreeLists[i] = nullptr;
    }

    // reset main free list and mapping trackers (keep existing mapped pages)
    freeList = nullptr;

    // rebuild a single free block over all currently mapped pages
    UInt32 mapped = _heapMappedBytes;

    if (mapped < _heapPageSize) {
      // ensure at least one page is mapped
      UInt8* newPage = MapNextHeapPage();
      (void)newPage;
      mapped = _heapMappedBytes;
    }

    FreeBlock* block = reinterpret_cast<FreeBlock*>(_heapBase);
    block->size = mapped - sizeof(FreeBlock);
    block->next = nullptr;
    SetFreeBlockCanary(block);

    freeList = block;

    CoalesceAdjacentFreeBlocks();
    ReclaimPageSpans();

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Heap reset: mapped=%p freeBytes=%p",
      _heapMappedBytes,
      block->size
    );
  }
}
