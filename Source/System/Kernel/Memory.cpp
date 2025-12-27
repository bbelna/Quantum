/**
 * @file System/Kernel/Memory.cpp
 * @brief Kernel memory management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Align.hpp>
#include <Types.hpp>

#include "Arch/Memory.hpp"
#include "Logger.hpp"
#include "Memory.hpp"
#include "Panic.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel {
  using ::Quantum::AlignDown;
  using ::Quantum::AlignUp;
  using LogLevel = Kernel::Logger::Level;

  UInt32 Memory::_heapStartVirtualAddress = Arch::Memory::kernelHeapBase;
  UInt32 Memory::_heapRegionBytes = Arch::Memory::kernelHeapBytes;

  void Memory::SetFreeBlockCanary(Memory::FreeBlock* block) {
    if (block->size < sizeof(UInt32)) {
      PANIC("Free block too small for canary");
    }

    UInt8* payload
      = reinterpret_cast<UInt8*>(block) + sizeof(Memory::FreeBlock);
    UInt32 usable = block->size - sizeof(UInt32);
    UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

    *canary = _canaryValue;
  }

  UInt8* Memory::MapNextHeapPage() {
    UInt32 heapLimit = _heapStartVirtualAddress + _heapRegionBytes;
    UInt32 nextEnd = reinterpret_cast<UInt32>(_heapMappedEnd)
      + _heapPageSize
      + _heapGuardPagesAfter * _heapPageSize;

    if (nextEnd > heapLimit) {
      PANIC("Kernel heap region exhausted");
    }

    UInt8* pageStart = _heapMappedEnd;
    void* physicalPageAddress = Arch::Memory::AllocatePage(true);

    Arch::Memory::MapPage(
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

  void Memory::EnsureHeapInitialized() {
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

  void Memory::CoalesceAdjacentFreeBlocks() {
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

  void Memory::ReclaimPageSpans() {
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
      AlignUp(
        reinterpret_cast<UInt32>(blockPayload),
        _heapPageSize
      )
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
      UInt32 pageTableEntry = Arch::Memory::GetPageTableEntry(virtualPage);

      if ((pageTableEntry & 0x1) == 0) {
        continue;
      }

      UInt32 physical = pageTableEntry & ~0xFFFu;

      Arch::Memory::UnmapPage(virtualPage);

      if (physical) {
        Arch::Memory::FreePage(reinterpret_cast<void*>(physical));
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

  void Memory::InsertFreeBlockSorted(Memory::FreeBlock* block) {
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

  void* Memory::AllocateFromFreeList(UInt32 needed) {
    Memory::FreeBlock* previous = nullptr;
    Memory::FreeBlock* current = _freeList;

    while (current) {
      // sanity: block must fit within mapped heap
      UInt8* blockStart = reinterpret_cast<UInt8*>(current);
      UInt8* blockEnd = blockStart + sizeof(Memory::FreeBlock) + current->size;

      if (
        blockStart < _heapBase
        || blockEnd > _heapBase + _heapMappedBytes
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

  int Memory::BinIndexForSize(UInt32 size) {
    for (UInt32 i = 0; i < _binCount; ++i) {
      if (size <= _binSizes[i]) {
        return static_cast<int>(i);
      }
    }

    return -1;
  }

  UInt32 Memory::PayloadSizeFromBlock(UInt32 blockSize) {
    if (blockSize <= sizeof(UInt32)) {
      return 0;
    }

    return AlignDown(blockSize - sizeof(UInt32), 8);
  }

  void* Memory::AllocateFromBin(UInt32 binSize, UInt32 neededWithHeader) {
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

  void Memory::InsertIntoBinOrFreeList(Memory::FreeBlock* block) {
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

  void Memory::Initialize(UInt32 bootInfoPhysicalAddress) {
    Arch::Memory::InitializePaging(bootInfoPhysicalAddress);

    UInt64 totalBytes
      = static_cast<UInt64>(Arch::Memory::GetPhysicalAllocatorTotalPages())
      * _heapPageSize;
    UInt64 usedBytes
      = static_cast<UInt64>(Arch::Memory::GetPhysicalAllocatorUsedPages())
      * _heapPageSize;
    UInt64 freeBytes
      = static_cast<UInt64>(Arch::Memory::GetPhysicalAllocatorFreePages())
      * _heapPageSize;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Physical allocator: pages total=%p used=%p free=%p bytes total=%p "
        "used=%p free=%p",
      Arch::Memory::GetPhysicalAllocatorTotalPages(),
      Arch::Memory::GetPhysicalAllocatorUsedPages(),
      Arch::Memory::GetPhysicalAllocatorFreePages(),
      totalBytes,
      usedBytes,
      freeBytes
    );
    DumpState();
  }

  void* Memory::AllocatePage(bool zero) {
    return Arch::Memory::AllocatePage(zero);
  }

  UInt32 Memory::GetKernelPageDirectoryPhysical() {
    return Arch::Memory::GetKernelPageDirectoryPhysical();
  }

  void Memory::MapPage(
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    Arch::Memory::MapPage(
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  UInt32 Memory::CreateAddressSpace() {
    return Arch::Memory::CreateAddressSpace();
  }

  void Memory::DestroyAddressSpace(UInt32 pageDirectoryPhysical) {
    Arch::Memory::DestroyAddressSpace(pageDirectoryPhysical);
  }

  void Memory::MapPageInAddressSpace(
    UInt32 pageDirectoryPhysical,
    UInt32 virtualAddress,
    UInt32 physicalAddress,
    bool writable,
    bool user,
    bool global
  ) {
    Arch::Memory::MapPageInAddressSpace(
      pageDirectoryPhysical,
      virtualAddress,
      physicalAddress,
      writable,
      user,
      global
    );
  }

  void Memory::ActivateAddressSpace(UInt32 pageDirectoryPhysical) {
    Arch::Memory::ActivateAddressSpace(pageDirectoryPhysical);
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

    Logger::Write(LogLevel::Debug, "Allocate request");
    Logger::WriteFormatted(
      LogLevel::Debug,
      "  requested=%p binIndex=%d binSize=%p",
      requested,
      binIndex,
      binSize
    );
    Logger::WriteFormatted(
      LogLevel::Debug,
      "  payloadSize=%p needed=%p",
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
      Memory::FreeBlock* block
        = reinterpret_cast<Memory::FreeBlock*>(firstPage);

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

      Logger::Write(LogLevel::Debug, "Allocation successful");
      Logger::WriteFormatted(
        LogLevel::Debug,
        "  ptr=%p block=%p usable=%p",
        payload,
        reinterpret_cast<UInt8*>(payload) - sizeof(Memory::FreeBlock),
        usable
      );
      Logger::WriteFormatted(
        LogLevel::Debug,
        "  size=%p canary=%p mapped=%p",
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
    Arch::Memory::FreePage(page);
  }

  void Memory::Free(void* pointer) {
    if (!pointer) return;

    UInt8* bytePointer = reinterpret_cast<UInt8*>(pointer);

    if (
      bytePointer < _heapBase
      || bytePointer >= _heapBase + _heapMappedBytes
    ) {
      PANIC("Heap free: pointer out of range");
    }

    Memory::FreeBlock* block = reinterpret_cast<Memory::FreeBlock*>(
      bytePointer - sizeof(Memory::FreeBlock)
    );
    UInt8* blockBytes = reinterpret_cast<UInt8*>(block);
    UInt8* payload
      = reinterpret_cast<UInt8*>(block) + sizeof(Memory::FreeBlock);

    // if the pointer is not at the block payload start, it may be an aligned
    // allocation; verify metadata before using it
    if (
      bytePointer != payload
      && bytePointer >= _heapBase + sizeof(Memory::AlignedMetadata)
    ) {
      Memory::AlignedMetadata* metadata
        = reinterpret_cast<Memory::AlignedMetadata*>(bytePointer) - 1;

      if (metadata->magic == _alignedMagic) {
        UInt8* candidateBlockBytes
          = reinterpret_cast<UInt8*>(metadata->block);

        if (
          candidateBlockBytes >= _heapBase
          && candidateBlockBytes < _heapBase + _heapMappedBytes
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
    Memory::HeapState state {};

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
