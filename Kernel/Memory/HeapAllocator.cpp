/**
 * @file Kernel/Memory/HeapAllocator.cpp
 * @brief Implements @ref @QKrnl::Memory::HeapAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>

#include "HeapAllocator.hpp"
#include "MemoryPressure.hpp"

namespace Quantum::Kernel::Memory {
  void HeapAllocator::Initialize(
    IMemoryAllocator* memoryAllocator,
    IMemoryMapper* memoryMapper,
    IMaydayHandler* maydayHandler,
    MemoryPressureMonitor* pressure,
    HeapAllocatorConfiguration configuration
  ) {
    if (!_base) {
      _memoryAllocator = memoryAllocator;
      _memoryMapper = memoryMapper;
      _maydayHandler = maydayHandler;
      _pressure = pressure;

      _addressSpace = configuration.HeapAddressSpace;
      _block = configuration.HeapBlock;
      _guardBlockCountBefore = configuration.GuardBlocksBefore;
      _guardBlockCountAfter = configuration.GuardBlocksAfter;
      _allocatedPoison = configuration.AllocatedPoison;
      _freedPoison = configuration.FreedPoison;
      _canary = configuration.Canary;
      _allocatedSentinel = configuration.AllocatedSentinel;
      _requiredTailBlocks
        = configuration.RequiredTailBlocks < MinimumTailBlocks
        ? MinimumTailBlocks
        : configuration.RequiredTailBlocks;

      _base = reinterpret_cast<UInt8*>(
        _block.Base + (
          static_cast<UIntPtr>(_guardBlockCountBefore) *
          _memoryAllocator->GetBlockSize()
        )
      );
      _current = _base;
      _mappedEnd = _base;
      _mappedBytes = 0;
      _guardAddress = _base + (
        static_cast<UIntPtr>(_block.SizeInBytes) -
        (static_cast<UIntPtr>(_guardBlockCountAfter) *
         _memoryAllocator->GetBlockSize())
      );
      _freeList = nullptr;

      KLOG_TRACE(
        "HeapAllocator initialized for address space %p with region %p-%p",
        reinterpret_cast<UIntPtr>(_addressSpace),
        _block.Base,
        _block.Base + _block.SizeInBytes
      );
    }
  }

  void* HeapAllocator::Allocate(Size size) {
    Size requested = static_cast<Size>(
      AlignUp(
        static_cast<UIntPtr>(size),
        8
      )
    );
    int binIndex = _binIndexForSize(requested);
    Size binSize
      = binIndex >= 0
      ? _binSizes[binIndex]
      : requested;
    Size payloadSize = static_cast<Size>(
      AlignUp(
        static_cast<UIntPtr>(binSize + sizeof(UInt32)),
        8
      )
    ); // space for canary
    Size needed = payloadSize + sizeof(FreeHeapBlock);
    Size blocksNeeded
      = (needed + _memoryAllocator->GetBlockSize() - 1)
      / _memoryAllocator->GetBlockSize();

    if (blocksNeeded > _requiredTailBlocks) {
      _requiredTailBlocks = blocksNeeded;
    }

    void* pointer = nullptr;

    while (true) {
      if (binIndex >= 0) {
        pointer = _allocateFromBin(
          binSize,
          needed
        );
      } else {
        pointer = _allocateFromFreeList(needed);
      }

      if (pointer) {
        break;
      }

      // map enough contiguous blocks to satisfy this allocation in a single
      // block
      Size blocksToMap
        = (needed + _memoryAllocator->GetBlockSize() - 1)
        / _memoryAllocator->GetBlockSize();

      if (blocksToMap == 0) {
        blocksToMap = 1;
      }

      UInt8* firstBlock = _mapNextBlock();

      for (Size i = 1; i < blocksToMap; ++i) {
        _mapNextBlock();
      }

      Size totalBytes = blocksToMap * _memoryAllocator->GetBlockSize();
      FreeHeapBlock* block = reinterpret_cast<FreeHeapBlock*>(firstBlock);

      block->SizeInBytes = totalBytes - sizeof(FreeHeapBlock);
      block->Next = nullptr;

      _setFreeHeapBlockCanary(block);
      _insertFreeHeapBlockSorted(block);
    }

    if (pointer) {
      UInt8* payload = reinterpret_cast<UInt8*>(pointer);
      FreeHeapBlock* block = reinterpret_cast<FreeHeapBlock*>(
        payload - sizeof(FreeHeapBlock)
      );

      if (block->SizeInBytes < sizeof(UInt32)) {
        MAYDAY("Heap allocator block too small for canary");
      }

      block->Next = reinterpret_cast<FreeHeapBlock*>(_allocatedSentinel);

      Size usable = block->SizeInBytes - sizeof(UInt32);

      #ifdef HEAP_DEBUG
      for (Size index = 0; index < usable; ++index) {
        payload[index] = _allocatedPoison;
      }
      #endif

      UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

      *canary = _canary;

      KLOG_DEBUG(
        "Heap allocated block %p of size %u bytes (usable %u bytes)",
        pointer,
        static_cast<UInt32>(block->SizeInBytes),
        static_cast<UInt32>(usable)
      );

      return pointer;
    }

    MAYDAY("Heap allocator failed to allocate memory");

    return nullptr;
  }

  void HeapAllocator::Free(void* pointer) {
    if (pointer) {
      UInt8* bytePointer = reinterpret_cast<UInt8*>(pointer);

      if (
        bytePointer < _base ||
        bytePointer >= _base + _mappedBytes
      ) {
        MAYDAY("Heap free pointer out of bounds");
      }

      FreeHeapBlock* block = reinterpret_cast<FreeHeapBlock*>(
        bytePointer - sizeof(FreeHeapBlock)
      );
      UInt8* blockBytes = reinterpret_cast<UInt8*>(block);
      UInt8* payload
        = reinterpret_cast<UInt8*>(block)
        + sizeof(FreeHeapBlock);

      // if the pointer is not at the block payload start, it may be an aligned
      // allocation; verify metadata before using it
      if (
        bytePointer != payload &&
        bytePointer >= _base + sizeof(AlignedHeapAllocationHeader)
      ) {
        AlignedHeapAllocationHeader* metadata
          = reinterpret_cast<AlignedHeapAllocationHeader*>(bytePointer)
          - 1;

        if (metadata->Magic == _alignedMagic) {
          UInt8* candidateBlockBytes = reinterpret_cast<UInt8*>(
            metadata->Block
          );

          if (
            candidateBlockBytes >= _base &&
            candidateBlockBytes < _base + _mappedBytes
          ) {
            FreeHeapBlock* candidateBlock = metadata->Block;
            UInt8* candidatePayload
              = candidateBlockBytes
              + sizeof(FreeHeapBlock);
            UInt8* candidateAligned
              = candidatePayload
              + metadata->PayloadOffset;
            UInt8* candidateEnd
              = candidatePayload
              + candidateBlock->SizeInBytes;
            UInt8* metadataBytes = reinterpret_cast<UInt8*>(metadata);

            bool metadataValid
              = metadata->PayloadOffset < candidateBlock->SizeInBytes
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

      if (
        blockBytes < _base ||
        blockBytes >= _base + _mappedBytes
      ) {
        MAYDAY("Heap free block out of bounds");
      }

      // Pointer to the end of the block's payload for bounds checking.
      UInt8* blockEnd = payload + block->SizeInBytes;

      if (blockEnd > _base + _mappedBytes) {
        MAYDAY("Heap free block size out of bounds");
      } else if (block->SizeInBytes < sizeof(UInt32)) {
        MAYDAY("Heap free block too small for canary");
      } else if (
        block->Next != reinterpret_cast<FreeHeapBlock*>(_allocatedSentinel)
      ) {
        KLOG_ERROR(
          "Block %p has invalid next pointer %p",
          reinterpret_cast<UIntPtr>(block),
          reinterpret_cast<UIntPtr>(block->Next)
        );
        MAYDAY("Heap free block state invalid");
      }

      Size offset
        = bytePointer > payload
        ? static_cast<Size>(bytePointer - payload)
        : 0;

      if (offset >= block->SizeInBytes) {
        MAYDAY("Heap free offset beyond block size");
      }

      Size usable = block->SizeInBytes - offset;

      if (usable < sizeof(UInt32)) {
        MAYDAY("Heap free block too small for canary after offset");
      }

      usable -= sizeof(UInt32);

      UInt8* alignedPayload = payload + offset;
      UInt32* canary = reinterpret_cast<UInt32*>(alignedPayload + usable);

      if (*canary != _canary) {
        MAYDAY("Heap free canary corrupted");
      }

      #ifdef HEAP_DEBUG
      // maximum number of blocks to scan in free lists to detect double frees
      // or corruption
      const int maxScan = 16384;

      // checks a free list for the presence of a block, indicating a double
      // free, and also guards against infinite loops from list corruption
      auto checkList = [&](
        FreeHeapBlock* head,
        const char* name
      ) {
        int count = 0;

        while (head) {
          if (head == block) {
            KLOG_ERROR(
              "Double free detected for block %p in %s list",
              reinterpret_cast<UIntPtr>(block),
              name
            );
            MAYDAY("Heap free double free");
          }

          head = head->Next;
          ++count;

          if (count > maxScan) {
            KLOG_ERROR(
              "Excessive scan of %s list during free",
              name
            );
            MAYDAY("Heap free list corruption or cycle detected");
          }
        }
      };

      checkList(_freeList, "free");

      for (UInt32 i = 0; i < BinCount; ++i) {
        checkList(_binFreeLists[i], "bin");
      }

      for (Size i = 0; i < usable; ++i) {
        alignedPayload[i] = _freedPoison;
      }
      #endif

      _insertIntoBinOrFreeList(block);
    }
  }

  UInt8* HeapAllocator::_mapNextBlock() {
    // ensure we don't exceed the heap region limit
    UIntPtr heapLimit
      = _block.Base
      + _block.SizeInBytes
      - (
        static_cast<UIntPtr>(_guardBlockCountAfter) *
        _memoryAllocator->GetBlockSize()
      );
    UIntPtr nextEnd
      = reinterpret_cast<UIntPtr>(_mappedEnd)
      + static_cast<UIntPtr>(_memoryAllocator->GetBlockSize());

    // mayday if we would exceed the limit
    if (nextEnd > heapLimit) {
      MAYDAY("Kernel heap out of memory");
    }

    // set up the new block
    UInt8* blockStart = _mappedEnd;
    MemoryBlock kernelBlock = _memoryAllocator->Allocate(
      MemoryBlockTag::HeapSlab
    );

    if (
      kernelBlock.SizeInBytes == 0 &&
      _pressure
    ) {
      _pressure->Reclaim(MemoryPressure::Elevated);

      kernelBlock = _memoryAllocator->Allocate(MemoryBlockTag::HeapSlab);
    }

    if (kernelBlock.SizeInBytes == 0 && _pressure) {
      _pressure->Reclaim(MemoryPressure::Critical);

      kernelBlock = _memoryAllocator->Allocate(MemoryBlockTag::HeapSlab);
    }

    if (kernelBlock.SizeInBytes == 0) {
      MAYDAY("Kernel heap out of kernel memory");
    }

    // and map it
    _memoryMapper->Map(
      *_addressSpace,
      MemoryBlock {
        reinterpret_cast<UIntPtr>(_mappedEnd),
        _memoryAllocator->GetBlockSize()
      },
      kernelBlock,
      {
        MemoryMappingPermissions::Read
          | MemoryMappingPermissions::Write,
        MemoryMappingCache::Default,
        MemoryMappingOptions::None
      }
    );

    // update heap state
    _mappedEnd += static_cast<UIntPtr>(_memoryAllocator->GetBlockSize());
    _mappedBytes += _memoryAllocator->GetBlockSize();

    return blockStart;
  }

  void HeapAllocator::_reclaimBlockSpans() {
    if (
      !_freeList ||
      !_memoryMapper ||
      !_memoryAllocator
    ) {
      return;
    }

    // find the highest-addressed free block (end of heap)
    FreeHeapBlock* previous = nullptr;
    FreeHeapBlock* current = _freeList;

    while (current->Next) {
      previous = current;
      current = current->Next;
    }

    UInt8* blockStart = reinterpret_cast<UInt8*>(current);
    UInt8* blockPayload = blockStart + sizeof(FreeHeapBlock);
    UInt8* blockEnd = blockPayload + current->SizeInBytes;
    UInt8* heapEnd = _base + _mappedBytes;

    // only reclaim if this block reaches the mapped end of the heap
    if (blockEnd != heapEnd) {
      return;
    }

    UIntPtr blockPayloadAddress = reinterpret_cast<UIntPtr>(blockPayload);
    UIntPtr reclaimStartAddress = AlignUp(
      blockPayloadAddress,
      static_cast<UIntPtr>(_memoryAllocator->GetBlockSize())
    );
    UInt8* reclaimStart = reinterpret_cast<UInt8*>(reclaimStartAddress);

    if (reclaimStart >= heapEnd) {
      return;
    }

    Size reclaimableBlocks = static_cast<Size>(
      (heapEnd - reclaimStart) / _memoryAllocator->GetBlockSize()
    );

    if (_requiredTailBlocks < MinimumTailBlocks) {
      _requiredTailBlocks = MinimumTailBlocks;
    }

    Size reserveTailBlocks = _requiredTailBlocks;

    if (reclaimableBlocks <= reserveTailBlocks) {
      return;
    }

    Size blocksToReclaim = reclaimableBlocks - reserveTailBlocks;
    Size bytesToReclaim
      = blocksToReclaim
      * _memoryAllocator->GetBlockSize();

    for (Size i = 0; i < blocksToReclaim; ++i) {
      UIntPtr processBlockBase
        = reinterpret_cast<UIntPtr>(heapEnd)
        - static_cast<UIntPtr>(
            (i + 1) * _memoryAllocator->GetBlockSize()
          );

      MemoryBlock processBlock {
        processBlockBase,
        _memoryAllocator->GetBlockSize()
      };

      MemoryBlock kernelBlock = _memoryMapper->Unmap(
        *_addressSpace,
        processBlock
      );

      if (kernelBlock.SizeInBytes == _memoryAllocator->GetBlockSize()) {
        _memoryAllocator->Free(kernelBlock);
      }
    }

    if (_mappedBytes >= bytesToReclaim) {
      _mappedBytes -= bytesToReclaim;
    } else {
      _mappedBytes = 0;
    }

    if (_mappedEnd >= _base + bytesToReclaim) {
      _mappedEnd -= bytesToReclaim;
    } else {
      _mappedEnd = _base;
    }

    // shrink the tail block to the remaining bytes before the reclaimed span
    UInt8* newHeapEnd = _base + _mappedBytes;
    Size newSize = static_cast<Size>(newHeapEnd - blockPayload);

    if (newSize < sizeof(UInt32)) {
      // drop the block if too small to hold a canary
      if (previous) {
        previous->Next = nullptr;
      } else {
        _freeList = nullptr;
      }
    } else {
      current->SizeInBytes = newSize;
      current->Next = nullptr;

      _setFreeHeapBlockCanary(current);
    }
  }

  void* HeapAllocator::_allocateFromFreeList(Size needed) {
    FreeHeapBlock* previous = nullptr;
    FreeHeapBlock* current = _freeList;

    while (current) {
      // sanity: block must fit within mapped heap
      UInt8* blockStart = reinterpret_cast<UInt8*>(current);
      UInt8* blockEnd
        = blockStart
        + sizeof(FreeHeapBlock)
        + current->SizeInBytes;

      if (
        blockStart < _base ||
        blockEnd > _base + _mappedBytes
      ) {
        MAYDAY("Heap free list corrupted");
      }

      Size total = current->SizeInBytes + sizeof(FreeHeapBlock);

      if (total >= needed) {
        // split if enough space remains for another block
        if (total >= needed + sizeof(FreeHeapBlock) + 8) {
          UInt8* newBlockAddress = reinterpret_cast<UInt8*>(current) + needed;
          FreeHeapBlock* newBlock = reinterpret_cast<FreeHeapBlock*>(
            newBlockAddress
          );

          newBlock->SizeInBytes = total - needed - sizeof(FreeHeapBlock);
          newBlock->Next = current->Next;

          _setFreeHeapBlockCanary(newBlock);

          current->SizeInBytes = needed - sizeof(FreeHeapBlock);
          current->Next = nullptr;

          if (previous) {
            previous->Next = newBlock;
          } else {
            _freeList = newBlock;
          }
        } else {
          // remove entire block
          if (previous) {
            previous->Next = current->Next;
          } else {
            _freeList = current->Next;
          }
        }

        return reinterpret_cast<UInt8*>(current) + sizeof(FreeHeapBlock);
      }

      previous = current;
      current = current->Next;
    }

    return nullptr;
  }

  void* HeapAllocator::_allocateFromBin(
    Size binSize,
    Size neededWithHeader
  ) {
    int index = _binIndexForSize(binSize);

    if (index < 0) {
      return nullptr;
    } else {
      if (_binFreeLists[index]) {
        FreeHeapBlock* block = _binFreeLists[index];

        if (block->Next == block) {
          KLOG_ERROR(
            "Bin list cycle bin %d block=%p",
            index,
            reinterpret_cast<UIntPtr>(block)
          );
          MAYDAY("Heap bin free list corrupted");
        }

        if (
          block->Next == reinterpret_cast<FreeHeapBlock*>(_allocatedSentinel)
        ) {
          KLOG_ERROR(
            "Allocated block in bin %d block=%p",
            index,
            reinterpret_cast<UIntPtr>(block)
          );
          MAYDAY("Heap bin free list corrupted");
        }

        UInt8* blockBytes = reinterpret_cast<UInt8*>(block);
        UInt8* blockEnd
          = blockBytes
          + sizeof(FreeHeapBlock)
          + block->SizeInBytes;

        if (
          blockBytes < _base ||
          blockEnd > _base + _mappedBytes
        ) {
          KLOG_ERROR(
            "Block out of bounds bin %d block=%p end=%p",
            index,
            reinterpret_cast<UIntPtr>(blockBytes),
            reinterpret_cast<UIntPtr>(blockEnd)
          );
          MAYDAY("Heap bin free list corrupted");
        }

        _binFreeLists[index] = block->Next;

        Size totalBytes = block->SizeInBytes + sizeof(FreeHeapBlock);

        if (totalBytes < neededWithHeader) {
          KLOG_ERROR(
            "Undersized block for bin %d size=%p needed=%p",
            index,
            static_cast<UIntPtr>(block->SizeInBytes),
            static_cast<UIntPtr>(neededWithHeader)
          );

          _insertFreeHeapBlockSorted(block);

          return _allocateFromFreeList(neededWithHeader);
        }

        return reinterpret_cast<UInt8*>(block) + sizeof(FreeHeapBlock);
      }

      // fallback to general free list
      void* pointer = _allocateFromFreeList(neededWithHeader);

      return pointer;
    }
  }

  void HeapAllocator::_setFreeHeapBlockCanary(FreeHeapBlock* block) {
    if (block->SizeInBytes < sizeof(UInt32)) {
      MAYDAY("Heap free block too small to hold canary");
    }

    // set the canary at the end of the block payload
    UInt8* payload
      = reinterpret_cast<UInt8*>(block)
      + sizeof(FreeHeapBlock);
    Size usable = block->SizeInBytes - sizeof(UInt32);
    UInt32* canary = reinterpret_cast<UInt32*>(payload + usable);

    // set the canary value
    *canary = _canary;
  }

  void HeapAllocator::_insertFreeHeapBlockSorted(FreeHeapBlock* block) {
    if (!_freeList || block < _freeList) {
      block->Next = _freeList;
      _freeList = block;
    } else {
      FreeHeapBlock* current = _freeList;

      while (current->Next && current->Next < block) {
        current = current->Next;
      }

      block->Next = current->Next;
      current->Next = block;
    }

    _coalesceAdjacentFreeHeapBlocks();
    _reclaimBlockSpans();
  }

  void HeapAllocator::_insertIntoBinOrFreeList(FreeHeapBlock* block) {
    Size payloadSize = _payloadSizeFromBlock(block->SizeInBytes);
    int index
      = payloadSize > 0
      ? _binIndexForSize(payloadSize)
      : -1;

    // verify the block can actually satisfy an allocation from the selected
    // bin. The bin's allocation requires AlignUp(binSize + canary, 8) + header
    // bytes, which may exceed the block's total size due to canary rounding
    // walk down to a smaller bin if needed
    while (index >= 0) {
      Size binPayload = static_cast<Size>(
        AlignUp(
          static_cast<UIntPtr>(_binSizes[index] + sizeof(UInt32)),
          8
        )
      );
      Size binNeeded = binPayload + sizeof(FreeHeapBlock);

      if (block->SizeInBytes + sizeof(FreeHeapBlock) >= binNeeded) {
        break;
      }

      index--;
    }

    if (index >= 0) {
      if (block->Next == block) {
        KLOG_ERROR(
          "Self-linked block=%p",
          reinterpret_cast<UIntPtr>(block)
        );
        MAYDAY("Heap bin free list corrupted");
      }

      block->Next = _binFreeLists[index];
      _binFreeLists[index] = block;

      _setFreeHeapBlockCanary(block);
    } else {
      _insertFreeHeapBlockSorted(block);
    }
  }

  void HeapAllocator::_coalesceAdjacentFreeHeapBlocks() {
    FreeHeapBlock* current = _freeList;

    while (current && current->Next) {
      UInt8* currentEnd
        = reinterpret_cast<UInt8*>(current)
        + sizeof(FreeHeapBlock)
        + current->SizeInBytes;

      if (currentEnd == reinterpret_cast<UInt8*>(current->Next)) {
        current->SizeInBytes
          += sizeof(FreeHeapBlock)
           + current->Next->SizeInBytes;
        current->Next = current->Next->Next;

        // only refresh the canary for the block that grew
        _setFreeHeapBlockCanary(current);
      } else {
        current = current->Next;
      }
    }
  }

  int HeapAllocator::_binIndexForSize(Size size) {
    for (
      UInt32 binIndex = 0;
      binIndex < BinCount;
      ++binIndex
    ) {
      if (size <= _binSizes[binIndex]) {
        return static_cast<int>(binIndex);
      }
    }

    return -1;
  }

  Size HeapAllocator::_payloadSizeFromBlock(Size blockSize) {
    if (blockSize <= sizeof(UInt32)) return 0;

    return static_cast<Size>(
      AlignDown(
        static_cast<UIntPtr>(blockSize - sizeof(UInt32)),
        8
      )
    );
  }
}
