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
#include <Types.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Memory.hpp>
#endif

namespace Quantum::System::Kernel {
  #if defined(QUANTUM_ARCH_IA32)
  using ArchPhysicalAllocatorState = Arch::IA32::Memory::PhysicalAllocatorState;
  using ArchMemory = Arch::IA32::Memory;
  using ArchCPU = Arch::IA32::CPU;
  #endif

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
     * Magic tag placed before aligned allocations.
     */
    constexpr UInt32 _alignedMagic = 0xA11A0CED;

    /**
     * Number of fixed-size bins.
     */
    constexpr UInt32 _binCount = 4;

    /**
     * Sizes of fixed-size bins.
     */
    constexpr UInt32 _binSizes[_binCount] = { 16, 32, 64, 128 };

    /**
     * Poison pattern used to fill newly allocated payloads.
     */
    constexpr UInt8 _poisonAllocated = 0xAA;

    /**
     * Poison pattern used to fill freed payloads.
     */
    constexpr UInt8 _poisonFreed = 0x55;

    /**
     * Canary value stored at the end of each allocation.
     */
    constexpr UInt32 _canaryValue = 0xDEADC0DE;

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
     * Head of the general free list.
     */
    Memory::FreeBlock* _freeList = nullptr;

    /**
     * Free lists for each fixed-size bin.
     */
    Memory::FreeBlock* _binFreeLists[_binCount]
      = { nullptr, nullptr, nullptr, nullptr };

    /**
     * Aligns a value to the next multiple of alignment.
     * @param value
     *   Value to align.
     * @param alignment
     *   Alignment boundary (power of two).
     * @return
     *   Aligned value.
     */
    inline UInt32 AlignUp(UInt32 value, UInt32 alignment) {
      return (value + alignment - 1) & ~(alignment - 1);
    }

    /**
     * Aligns a value down to the nearest alignment boundary.
     * @param value
     *   Value to align.
     * @param alignment
     *   Alignment boundary (power of two).
     * @return
     *   Aligned value at or below input.
     */
    inline UInt32 AlignDown(UInt32 value, UInt32 alignment) {
      return value & ~(alignment - 1);
    }

    /**
     * Writes the canary for a free block at the end of its payload.
     * @param block
     *   Block to write canary for.
     */
    inline void SetFreeBlockCanary(Memory::FreeBlock* block) {
      if (block->size < sizeof(UInt32)) {
        PANIC("Free block too small for canary");
      }

      UInt8* payload
        = reinterpret_cast<UInt8*>(block) + sizeof(Memory::FreeBlock);
      UInt32 usable = block->size - sizeof(UInt32);
      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

      *canary = _canaryValue;
    }

    /**
     * Maps the next page in the heap virtual range, keeping a guard page
     * unmapped immediately after the mapped region.
     * @return
     *   Pointer to the start of the mapped page.
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
        _freeList = nullptr;
      }
    }

    /**
     * Merges adjacent free blocks to reduce fragmentation.
     */
    void CoalesceAdjacentFreeBlocks() {
      Memory::FreeBlock* current = _freeList;

      while (current && current->next) {
        UInt8* currentEnd
          = reinterpret_cast<UInt8*>(current)
          + sizeof(Memory::FreeBlock)
          + current->size;

        if (currentEnd == reinterpret_cast<UInt8*>(current->next)) {
          current->size += sizeof(Memory::FreeBlock) + current->next->size;
          current->next = current->next->next;
        } else {
          current = current->next;
        }
      }

      // refresh canaries for all free blocks after any coalescing
      current = _freeList;

      while (current) {
        SetFreeBlockCanary(current);

        current = current->next;
      }
    }

    /**
     * Reclaims pages from the end of the heap so the mapped region remains
     * contiguous.
     */
    void ReclaimPageSpans() {
      if (!_freeList) {
        return;
      }

      // find the highest-addressed free block (end of heap)
      Memory::FreeBlock* previous = nullptr;
      Memory::FreeBlock* current = _freeList;

      while (current->next) {
        previous = current;
        current = current->next;
      }

      UInt8* blockStart = reinterpret_cast<UInt8*>(current);
      UInt8* blockPayload = blockStart + sizeof(Memory::FreeBlock);
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
        UInt32 pageTableEntry = ArchMemory::GetPageTableEntry(virtualPage);

        if ((pageTableEntry & 0x1) == 0) {
          continue;
        }

        UInt32 physical = pageTableEntry & ~0xFFFu;

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
          _freeList = nullptr;
        }
      } else {
        current->size = newSize;
        current->next = nullptr;

        SetFreeBlockCanary(current);
      }
    }

    /**
     * Inserts a free block into the sorted free list and coalesces neighbors.
     * @param block
     *   Block to insert.
     */
    void InsertFreeBlockSorted(Memory::FreeBlock* block) {
      if (!_freeList || block < _freeList) {
        block->next = _freeList;
        _freeList = block;
      } else {
        Memory::FreeBlock* current = _freeList;

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
     * @param needed
     *   Total bytes requested including header.
     * @return
     *   Pointer to payload or `nullptr` if none fit.
     */
    void* AllocateFromFreeList(UInt32 needed) {
      Memory::FreeBlock* previous = nullptr;
      Memory::FreeBlock* current = _freeList;

      while (current) {
        // sanity: block must fit within mapped heap
        UInt8* blockStart = reinterpret_cast<UInt8*>(current);
        UInt8* blockEnd = blockStart + sizeof(Memory::FreeBlock) + current->size;

        if (
          blockStart < _heapBase ||
          blockEnd > _heapBase + _heapMappedBytes
        ) {
          Logger::Write(
            LogLevel::Error,
            "AllocateFromFreeList: corrupt block"
          );
          Logger::WriteFormatted(
            LogLevel::Error,
            "addr=%p size=%p end=%p (heap base=%p end=%p needed=%p)",
            blockStart,
            current->size,
            blockEnd,
            _heapBase,
            _heapBase + _heapMappedBytes,
            needed
          );
          
          // dump entire free list
          Logger::Write(LogLevel::Error, "Free list dump:");

          Memory::FreeBlock* dump = _freeList;
          int count = 0;
          
          while (dump && count < 20) {
            UInt8* dumpStart = reinterpret_cast<UInt8*>(dump);
            UInt8* dumpEnd = dumpStart + sizeof(Memory::FreeBlock) + dump->size;
            
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

        UInt32 total = current->size + sizeof(Memory::FreeBlock);
        if (total >= needed) {
          // split if enough space remains for another block
          if (total >= needed + sizeof(Memory::FreeBlock) + 8) {
            UInt8* newBlockAddr = reinterpret_cast<UInt8*>(current) + needed;
            Memory::FreeBlock* newBlock
              = reinterpret_cast<Memory::FreeBlock*>(newBlockAddr);

            newBlock->size = total - needed - sizeof(Memory::FreeBlock);
            newBlock->next = current->next;

            SetFreeBlockCanary(newBlock);

            current->size = needed - sizeof(Memory::FreeBlock);
            current->next = nullptr;

            if (previous) {
              previous->next = newBlock;
            } else {
              _freeList = newBlock;
            }
          } else {
            // remove entire block
            if (previous) {
              previous->next = current->next;
            } else {
              _freeList = current->next;
            }
          }

          return reinterpret_cast<UInt8*>(current) + sizeof(Memory::FreeBlock);
        }

        previous = current;
        current = current->next;
      }

      return nullptr;
    }

    /**
     * Determines the bin index for a requested payload size.
     * @param size
     *   Payload bytes requested.
     * @return
     *   Bin index or -1 if it does not fit in a fixed bin.
     */
    int BinIndexForSize(UInt32 size) {
      for (UInt32 i = 0; i < _binCount; ++i) {
        if (size <= _binSizes[i]) {
          return static_cast<int>(i);
        }
      }

      return -1;
    }

    /**
     * Derives the original payload size (without canary/padding) from a stored
     * block size.
     * @param blockSize
     *   Size stored in the block header (payload + canary + padding).
     * @return
     *   Payload size rounded down to the allocator's 8-byte alignment.
     */
    UInt32 PayloadSizeFromBlock(UInt32 blockSize) {
      if (blockSize <= sizeof(UInt32)) {
        return 0;
      }

      return AlignDown(blockSize - sizeof(UInt32), 8);
    }

    /**
     * Allocates from a fixed-size bin if available, otherwise falls back to
     * free list.
     * @param binSize
     *   Bin payload size to request.
     * @param neededWithHeader
     *   Total bytes including header.
     * @return
     *   Pointer to payload or nullptr.
     */
    void* AllocateFromBin(UInt32 binSize, UInt32 neededWithHeader) {
      int index = BinIndexForSize(binSize);

      if (index < 0) {
        return nullptr;
      } else {
        if (_binFreeLists[index]) {
          Memory::FreeBlock* block = _binFreeLists[index];

          _binFreeLists[index] = block->next;

          UInt32 totalBytes = block->size + sizeof(Memory::FreeBlock);

          if (totalBytes < neededWithHeader) {
            Logger::WriteFormatted(
              LogLevel::Error,
              "AllocateFromBin: undersized block for bin index=%d "
                "blockSize=%p needed=%p",
              index,
              block->size,
              neededWithHeader
            );

            InsertFreeBlockSorted(block);

            return AllocateFromFreeList(neededWithHeader);
          }

          return reinterpret_cast<UInt8*>(block) + sizeof(Memory::FreeBlock);
        }

        // fallback to general free list
        void* pointer = AllocateFromFreeList(neededWithHeader);

        return pointer;
      }
    }

    /**
     * Returns a freed block either to a size bin or the general free list.
     * @param block
     *   Block being freed.
     */
    void InsertIntoBinOrFreeList(Memory::FreeBlock* block) {
      UInt32 payloadSize = PayloadSizeFromBlock(block->size);
      int index = (payloadSize > 0) ? BinIndexForSize(payloadSize) : -1;

      if (index >= 0) {
        block->next = _binFreeLists[index];
        _binFreeLists[index] = block;

        SetFreeBlockCanary(block);
      } else {
        InsertFreeBlockSorted(block);
      }
    }
  }

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    ArchMemory::InitializePaging(bootInfoPhysicalAddress);

    ArchPhysicalAllocatorState physicalState
      = ArchMemory::GetPhysicalAllocatorState();
    UInt64 totalBytes
      = static_cast<UInt64>(physicalState.totalPages) * _heapPageSize;
    UInt64 usedBytes
      = static_cast<UInt64>(physicalState.usedPages) * _heapPageSize;
    UInt64 freeBytes
      = static_cast<UInt64>(physicalState.freePages) * _heapPageSize;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Physical allocator: pages total=%p used=%p free=%p bytes total=%p "
        "used=%p free=%p",
      physicalState.totalPages,
      physicalState.usedPages,
      physicalState.freePages,
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

  void Memory::MapPage(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    ArchMemory::MapPage(
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  void* Memory::Allocate(Size size) {
    UInt32 requested = AlignUp(static_cast<UInt32>(size), 8);
    int binIndex = BinIndexForSize(requested);
    UInt32 binSize = (binIndex >= 0) ? _binSizes[binIndex] : requested;
    UInt32 payloadSize
      = AlignUp(binSize + sizeof(UInt32), 8); // space for canary
    UInt32 needed = payloadSize + sizeof(Memory::FreeBlock);
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

      // map enough contiguous pages to satisfy this allocation in a single
      // block
      UInt32 pagesToMap = (needed + _heapPageSize - 1) / _heapPageSize;

      if (pagesToMap == 0) {
        pagesToMap = 1;
      }

      UInt8* firstPage = MapNextHeapPage();

      for (UInt32 i = 1; i < pagesToMap; ++i) {
        MapNextHeapPage();
      }

      UInt32 totalBytes = pagesToMap * _heapPageSize;
      Memory::FreeBlock* block = reinterpret_cast<Memory::FreeBlock*>(firstPage);

      block->size = totalBytes - sizeof(Memory::FreeBlock);
      block->next = nullptr;

      SetFreeBlockCanary(block);
      InsertFreeBlockSorted(block);
    }

    if (pointer) {
      UInt8* payload = reinterpret_cast<UInt8*>(pointer);
      Memory::FreeBlock* blk = reinterpret_cast<Memory::FreeBlock*>(
        payload - sizeof(Memory::FreeBlock)
      );

      if (blk->size < sizeof(UInt32)) {
        PANIC("Heap alloc: block too small for canary");
      }

      UInt32 usable = blk->size - sizeof(UInt32);

      for (UInt32 i = 0; i < usable; ++i) {
        payload[i] = _poisonAllocated;
      }

      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

      *canary = _canaryValue;

      Logger::WriteFormatted(
        LogLevel::Debug,
        "Heap alloc ptr=%p block=%p usable=%p size=%p canary=%p mapped=%p",
        payload,
        reinterpret_cast<UInt8*>(payload) - sizeof(Memory::FreeBlock),
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

    UInt32 padding
    = static_cast<UInt32>(alignment) + sizeof(Memory::AlignedMetadata);
    void* raw = Allocate(size + padding);
    UInt8* rawBytes = reinterpret_cast<UInt8*>(raw);
    UIntPtr rawAddress = reinterpret_cast<UIntPtr>(rawBytes);
    UIntPtr alignedAddress = (rawAddress + alignment - 1) & ~(alignment - 1);
    Memory::AlignedMetadata* metadata
      = reinterpret_cast<Memory::AlignedMetadata*>(alignedAddress) - 1;

    metadata->magic = _alignedMagic;
    metadata->block = reinterpret_cast<Memory::FreeBlock*>(
      rawBytes - sizeof(Memory::FreeBlock)
    );
    metadata->payloadOffset
      = static_cast<UInt32>(alignedAddress - (rawAddress));

    UInt8* alignedPayload = reinterpret_cast<UInt8*>(alignedAddress);
    Memory::FreeBlock* block = metadata->block;
    UInt32 usable = block->size - metadata->payloadOffset;

    if (usable < sizeof(UInt32)) {
      PANIC("AllocateAligned: block too small for canary");
    }

    usable -= sizeof(UInt32);

    for (UInt32 i = 0; i < usable; ++i) {
      alignedPayload[i] = _poisonAllocated;
    }

    UInt32* canary = reinterpret_cast<UInt32*>(alignedPayload + usable);

    *canary = _canaryValue;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Heap alloc aligned ptr=%p block=%p payload=%p offset=%p usable=%p "
        "size=%p canary=%p",
      alignedPayload,
      block,
      reinterpret_cast<UInt8*>(block) + sizeof(Memory::FreeBlock),
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

    Memory::FreeBlock* block = reinterpret_cast<Memory::FreeBlock*>(
      bytePointer - sizeof(Memory::FreeBlock)
    );
    UInt8* blockBytes = reinterpret_cast<UInt8*>(block);
    UInt8* payload = reinterpret_cast<UInt8*>(block) + sizeof(Memory::FreeBlock);

    // if the pointer is not at the block payload start, it may be an aligned
    // allocation; verify metadata before using it
    if (
      bytePointer != payload &&
      bytePointer >= _heapBase + sizeof(Memory::AlignedMetadata)
    ) {
      Memory::AlignedMetadata* metadata
        = reinterpret_cast<Memory::AlignedMetadata*>(bytePointer) - 1;

      if (metadata->magic == _alignedMagic) {
        UInt8* candidateBlockBytes
          = reinterpret_cast<UInt8*>(metadata->block);

        if (
          candidateBlockBytes >= _heapBase &&
          candidateBlockBytes < _heapBase + _heapMappedBytes
        ) {
          Memory::FreeBlock* candidateBlock = metadata->block;
          UInt8* candidatePayload
            = candidateBlockBytes + sizeof(Memory::FreeBlock);
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

    UInt32 offset = (bytePointer > payload)
      ? static_cast<UInt32>(bytePointer - payload)
      : 0;

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

    if (*canary != _canaryValue) {
      Logger::WriteFormatted(
        LogLevel::Error,
        "Heap free: canary mismatch ptr=%p block=%p payload=%p offset=%p "
          "usable=%p size=%p canary=%p expected=%p",
        bytePointer,
        block,
        payload,
        offset,
        usable,
        block->size,
        *canary,
        _canaryValue
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
      alignedPayload[i] = _poisonFreed;
    }

    InsertIntoBinOrFreeList(block);
  }

  Memory::HeapState Memory::GetHeapState() {
    Memory::HeapState state{};

    UInt32 freeBytes = 0;
    UInt32 blocks = 0;
    Memory::FreeBlock* current = _freeList;

    while (current) {
      freeBytes += current->size;
      ++blocks;
      current = current->next;
    }

    state.mappedBytes = _heapMappedBytes;
    state.freeBytes = freeBytes;
    state.freeBlocks = blocks;

    return state;
  }

  void Memory::DumpState() {
    Memory::HeapState state = GetHeapState();

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

    Memory::HeapState before = GetHeapState();

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

    Memory::HeapState after = GetHeapState();

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
    Memory::FreeBlock* current = _freeList;
    Memory::FreeBlock* last = nullptr;

    while (current) {
      UInt8* blockBytes = reinterpret_cast<UInt8*>(current);
      UInt8* payload = blockBytes + sizeof(Memory::FreeBlock);
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
    current = _freeList;

    while (current) {
      UInt8* payload
        = reinterpret_cast<UInt8*>(current) + sizeof(Memory::FreeBlock);

      if (current->size < sizeof(UInt32)) {
        Logger::Write(
          LogLevel::Error,
          "VerifyHeap: free block too small for canary"
        );

        ok = false;

        break;
      }

      UInt32 usable = current->size - sizeof(UInt32);
      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

      if (*canary != _canaryValue) {
        Logger::Write(
          LogLevel::Error,
          "VerifyHeap: free block canary corrupted"
        );

        ok = false;

        break;
      }

      current = current->next;
    }

    // dump free list snapshot for debugging
    Logger::Write(LogLevel::Debug, "Free list dump:");

    current = _freeList;

    int count = 0;

    while (current && count < 20) {
      UInt8* blockStart = reinterpret_cast<UInt8*>(current);
      UInt8* blockEnd = blockStart + sizeof(Memory::FreeBlock) + current->size;

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
    for (UInt32 i = 0; i < _binCount; ++i) {
      _binFreeLists[i] = nullptr;
    }

    // reset main free list and mapping trackers (keep existing mapped pages)
    _freeList = nullptr;

    // rebuild a single free block over all currently mapped pages
    UInt32 mapped = _heapMappedBytes;

    if (mapped < _heapPageSize) {
      // ensure at least one page is mapped
      UInt8* newPage = MapNextHeapPage();
      (void)newPage;
      mapped = _heapMappedBytes;
    }

    Memory::FreeBlock* block = reinterpret_cast<Memory::FreeBlock*>(_heapBase);

    block->size = mapped - sizeof(Memory::FreeBlock);
    block->next = nullptr;

    SetFreeBlockCanary(block);

    _freeList = block;

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
