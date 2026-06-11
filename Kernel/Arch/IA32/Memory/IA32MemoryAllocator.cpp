/**
 * @file Kernel/Arch/IA32/Memory/IA32MemoryAllocator.cpp
 * @brief Implements @ref @QKrnlIA32::Memory::IA32MemoryAllocator.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/IA32Constants.hpp>
#include <Arch/IA32/IA32LinkerSymbols.hpp>
#include <Handlers/IOutOfMemoryHandler.hpp>
#include <KernelLog.hpp>

#include "IA32MemoryAllocator.hpp"
#include "IA32PageConstants.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  void IA32MemoryAllocator::Initialize(
    IMaydayHandler* maydayHandler,
    E820BootInfo* bootInfo
  ) {
    _maydayHandler = maydayHandler;
    _blockSize = PAGE_SIZE;
    _managedBytes = AlignUp(
      _calculateManagedBytes(bootInfo),
      _blockSize
    );
    _blockCount = _managedBytes / _blockSize;

    Size managedBytesMB = _managedBytes / MB;
    bool isManagedBytesExact = (managedBytesMB * MB) == _managedBytes;

    KLOG_INFO(
      "Managing %s%u MB of memory (BlockCount=%u, BlockSize=%u)",
      isManagedBytesExact
        ? ""
        : "~",
      managedBytesMB,
      _blockCount,
      _blockSize
    );

    _initialImageInterval = _calculateInitialImageInterval(bootInfo);

    _initializeAllocator(bootInfo);
  }

  void IA32MemoryAllocator::EnableTopDownAllocation() {
    _topDownEnabled = true;

    KLOG_DEBUG("Set _topDownEnabled=true");
  }

  void IA32MemoryAllocator::_initializeAllocator(E820BootInfo* bootInfo) {
    KLOG_DEBUG("Buddy init: start");

    // allocate metadata arrays at __phys_bss_end

    UInt32 refCountArraySize = _blockCount * sizeof(UInt32);
    UInt32 refCountPhysicalAddress = AlignUp(
      reinterpret_cast<UInt32>(&__phys_bss_end),
      sizeof(UInt32)
    );

    if (
      bootInfo &&
      bootInfo->InitialImageSizeInBytes > 0
    ) {
      UInt32 bundleStart = bootInfo->InitialImagePhysicalAddress;
      UInt32 bundleEnd = bundleStart + bootInfo->InitialImageSizeInBytes;
      UInt32 refCountEnd = refCountPhysicalAddress + refCountArraySize;

      if (
        refCountEnd > bundleStart &&
        refCountPhysicalAddress < bundleEnd
      ) {
        refCountPhysicalAddress = AlignUp(bundleEnd, sizeof(UInt32));
      }
    }

    _refCounts = reinterpret_cast<UInt32*>(refCountPhysicalAddress);

    // tag array after refcounts
    UInt32 tagArraySize = _blockCount * sizeof(MemoryBlockTag);
    UInt32 tagPhysicalAddress = AlignUp(
      refCountPhysicalAddress + refCountArraySize,
      sizeof(UInt32)
    );

    if (
      bootInfo &&
      bootInfo->InitialImageSizeInBytes > 0
    ) {
      UInt32 bundleStart = bootInfo->InitialImagePhysicalAddress;
      UInt32 bundleEnd = bundleStart + bootInfo->InitialImageSizeInBytes;
      UInt32 tagEnd = tagPhysicalAddress + tagArraySize;

      if (
        tagEnd > bundleStart &&
        tagPhysicalAddress < bundleEnd
      ) {
        tagPhysicalAddress = AlignUp(bundleEnd, sizeof(UInt32));
      }
    }

    _blockTags = reinterpret_cast<MemoryBlockTag*>(tagPhysicalAddress);

    // order array after tags
    UInt32 orderArraySize = _blockCount * sizeof(UInt8);
    UInt32 orderPhysicalAddress = AlignUp(
      tagPhysicalAddress + tagArraySize,
      sizeof(UInt32)
    );

    if (bootInfo && bootInfo->InitialImageSizeInBytes > 0) {
      UInt32 bundleStart = bootInfo->InitialImagePhysicalAddress;
      UInt32 bundleEnd = bundleStart + bootInfo->InitialImageSizeInBytes;
      UInt32 orderEnd = orderPhysicalAddress + orderArraySize;

      if (
        orderEnd > bundleStart &&
        orderPhysicalAddress < bundleEnd
      ) {
        orderPhysicalAddress = AlignUp(bundleEnd, sizeof(UInt32));
      }
    }

    UInt8* blockOrders = reinterpret_cast<UInt8*>(orderPhysicalAddress);

    KLOG_DEBUG(
      "Buddy init: metadata at refCounts=%p tags=%p orders=%p",
      refCountPhysicalAddress,
      tagPhysicalAddress,
      orderPhysicalAddress
    );

    // initialize the generic buddy allocator
    _buddy.Initialize(
      _maydayHandler,
      blockOrders,
      _blockCount,
      static_cast<UInt32>(_blockSize),
      _toNode
    );

    // pass 1: Initialize all arrays

    for (
      Size blockIndex = 0;
      blockIndex < _blockCount;
      ++blockIndex
    ) {
      _refCounts[blockIndex] = 0;
      _blockTags[blockIndex] = MemoryBlockTag::Unknown;
      blockOrders[blockIndex] = OrderReserved;
    }

    for (
      UInt8 tagIndex = 0;
      tagIndex < static_cast<UInt8>(MemoryBlockTag::Count);
      ++tagIndex
    ) {
      _tagCounts[tagIndex] = 0;
    }

    // mark usable E820 blocks as pending-free
    if (_blockFrameCount > 0) {
      for (
        Size blockFrameIndex = 0;
        blockFrameIndex < _blockFrameCount;
        ++blockFrameIndex
      ) {
        Interval<UInt32>& frame = _blockFrames[blockFrameIndex];
        UInt32 blockFrameStart = frame.Start;
        UInt32 blockFrameEnd = frame.End;

        if (blockFrameStart >= _blockCount) {
          continue;
        } else {
          if (blockFrameEnd > _blockCount) {
            blockFrameEnd = _blockCount;
          }

          for (
            UInt32 blockIndex = blockFrameStart;
            blockIndex < blockFrameEnd;
            ++blockIndex
          ) {
            blockOrders[blockIndex] = OrderPendingFree;
          }
        }
      }
    }

    KLOG_DEBUG("Buddy init: pass 1 complete, marking reserved regions");

    // re-mark reserved regions back to OrderReserved
    UInt32 kernelPhysicalEnd = reinterpret_cast<UInt32>(&__phys_end);

    _buddy.MarkReserved(0, kernelPhysicalEnd);
    _buddy.MarkReserved(refCountPhysicalAddress, refCountArraySize);
    _buddy.MarkReserved(tagPhysicalAddress, tagArraySize);
    _buddy.MarkReserved(orderPhysicalAddress, orderArraySize);
    _buddy.MarkReserved(0x90000, 0xA0000 - 0x90000); // boot stack

    if (
      bootInfo &&
      bootInfo->InitialImageSizeInBytes > 0
    ) {
      _buddy.MarkReserved(
        bootInfo->InitialImagePhysicalAddress,
        bootInfo->InitialImageSizeInBytes
      );
    }

    KLOG_DEBUG("Buddy init: reserved regions marked, starting pass 2");

    // pass 2: Build buddy free lists from pending-free blocks

    _usedBlockCount = 0;
    _reservedBytes = 0;

    UInt32 runStart = 0;
    bool inRun = false;

    for (
      UInt32 blockIndex = 0;
      blockIndex < _blockCount;
      ++blockIndex
    ) {
      if (blockOrders[blockIndex] == OrderPendingFree) {
        if (!inRun) {
          runStart = blockIndex;
          inRun = true;
        }
      } else {
        if (inRun) {
          KLOG_DEBUG(
            "Buddy init: free run [%u, %u) (%u blocks)",
            runStart,
            blockIndex,
            blockIndex - runStart
          );

          _buddy.ReturnBlocksToFreeLists(
            runStart,
            blockIndex - runStart
          );

          inRun = false;
        }

        // count reserved/allocated blocks
        ++_usedBlockCount;

        if (blockOrders[blockIndex] == OrderReserved) {
          _reservedBytes += _blockSize;
          _blockTags[blockIndex] = MemoryBlockTag::Reserved;
          ++_tagCounts[static_cast<UInt8>(MemoryBlockTag::Reserved)];
        }
      }
    }

    // flush trailing run
    if (inRun) {
      KLOG_DEBUG(
        "Buddy init: trailing free run [%u, %u) (%u blocks)",
        runStart,
        _blockCount,
        _blockCount - runStart
      );

      _buddy.ReturnBlocksToFreeLists(
        runStart,
        _blockCount - runStart
      );
    }

    KLOG_DEBUG("Buddy init: pass 2 complete");

    // compute initial image reserved bytes
    if (bootInfo && bootInfo->InitialImageSizeInBytes > 0) {
      UInt32 initialImageStart = _initialImageInterval.Start;
      UInt32 initialImageEnd = _initialImageInterval.End;

      if (initialImageEnd > initialImageStart) {
        _initialImageReservedBytes = 0;

        for (
          UInt32 blockIndex = initialImageStart;
          blockIndex < initialImageEnd && blockIndex < _blockCount;
          ++blockIndex
        ) {
          if (blockOrders[blockIndex] == OrderReserved) {
            _initialImageReservedBytes += _blockSize;
          }
        }
      }
    }

    Size freeBlocks = _blockCount - _usedBlockCount;

    KLOG_TRACE(
      "Buddy allocator initialized: %u free, %u used, %u reserved bytes",
      freeBlocks,
      _usedBlockCount,
      _reservedBytes
    );
    KLOG_TRACE(
      "Reference count array at %p (%u bytes for %u frames)",
      refCountPhysicalAddress,
      refCountArraySize,
      _blockCount
    );
    KLOG_TRACE(
      "Block tag array at %p (%u bytes for %u frames)",
      tagPhysicalAddress,
      tagArraySize,
      _blockCount
    );
    KLOG_TRACE(
      "Block order array at %p (%u bytes for %u frames)",
      orderPhysicalAddress,
      orderArraySize,
      _blockCount
    );
  }

  MemoryBlock IA32MemoryAllocator::Allocate() {
    UInt32 blockAddress = _buddy.AllocateBlock(_topDownEnabled);

    if (blockAddress == 0) {
      KLOG_CRITICAL(
        "Memory exhausted (%u/%u blocks used)",
        _usedBlockCount,
        _blockCount
      );

      if (_oomHandler && _oomHandler->Handle()) {
        return Allocate();
      }

      MAYDAY("Out Of Memory (OOM)");

      return {};
    }

    UInt32 blockIndex = blockAddress / _blockSize;

    _refCounts[blockIndex] = 1;
    ++_usedBlockCount;

    MemoryBlock allocatedBlock;

    allocatedBlock.Base = blockAddress;
    allocatedBlock.SizeInBytes = _blockSize;

    _zero(allocatedBlock);

    KLOG_DEBUG(
      "Allocated block %u at %p-%p, %u used blocks",
      blockIndex,
      allocatedBlock.Base,
      allocatedBlock.Base + allocatedBlock.SizeInBytes - 1,
      _usedBlockCount
    );

    return allocatedBlock;
  }

  MemoryBlock IA32MemoryAllocator::Allocate(Size size) {
    if (size == 0) {
      return {};
    }

    Size blocksNeeded = AlignUp(size, _blockSize) / _blockSize;

    if (blocksNeeded == 1) {
      return Allocate();
    }

    UInt32 blockAddress = _buddy.AllocateBlocks(
      static_cast<UInt32>(blocksNeeded),
      _topDownEnabled
    );

    if (blockAddress == 0) {
      if (_oomHandler && _oomHandler->Handle()) {
        return Allocate(size);
      }

      MAYDAY("Out of kernel blocks");

      return {};
    }

    UInt32 baseIndex = blockAddress / _blockSize;

    for (
      UInt32 blockIndex = 0;
      blockIndex < blocksNeeded;
      ++blockIndex
    ) {
      _refCounts[baseIndex + blockIndex] = 1;
    }

    _usedBlockCount += blocksNeeded;

    MemoryBlock allocatedBlock;

    allocatedBlock.Base = blockAddress;
    allocatedBlock.SizeInBytes = blocksNeeded * _blockSize;

    _zero(allocatedBlock);

    KLOG_DEBUG(
      "Allocated %u contiguous kernel blocks [%u, %u] at %p-%p, "
      "%u used blocks",
      blocksNeeded,
      baseIndex,
      baseIndex + blocksNeeded - 1,
      allocatedBlock.Base,
      allocatedBlock.Base + allocatedBlock.SizeInBytes - 1,
      _usedBlockCount
    );

    return allocatedBlock;
  }

  MemoryBlock IA32MemoryAllocator::AllocateBelow(UInt32 limit) {
    UInt32 blockAddress = _buddy.AllocateBlockBelow(limit);

    if (blockAddress == 0) {
      KLOG_WARNING(
        "No kernel blocks available below %p (%u/%u blocks used)",
        limit,
        _usedBlockCount,
        _blockCount
      );

      return {};
    }

    UInt32 blockIndex = blockAddress / _blockSize;

    _refCounts[blockIndex] = 1;
    ++_usedBlockCount;

    MemoryBlock allocatedBlock;

    allocatedBlock.Base = blockAddress;
    allocatedBlock.SizeInBytes = _blockSize;

    _zero(allocatedBlock);

    KLOG_DEBUG(
      "AllocateBelow(%p): block %u at %p-%p, %u used blocks",
      limit,
      blockIndex,
      allocatedBlock.Base,
      allocatedBlock.Base + allocatedBlock.SizeInBytes - 1,
      _usedBlockCount
    );

    return allocatedBlock;
  }

  void IA32MemoryAllocator::Free(MemoryBlock block) {
    if (block.SizeInBytes > 0) {
      // block should be aligned to the allocator's block size
      if (block.Base % _blockSize != 0) {
        KLOG_WARNING(
          "Cannot free unaligned block %p (%u bytes)",
          block.Base,
          block.SizeInBytes
        );

        return;
      }

      UInt32 startIndex = static_cast<UInt32>(block.Base / _blockSize);
      UInt32 blockCount = static_cast<UInt32>(
        AlignUp(
          block.SizeInBytes,
          _blockSize
        ) / _blockSize
      );

      if (blockCount == 0) blockCount = 1;

      UInt8* blockOrders = _buddy.GetBlockOrders();

      // validate range
      if (startIndex >= _blockCount) {
        KLOG_WARNING(
          "Cannot free out-of-bounds block %p (%u bytes)",
          block.Base,
          block.SizeInBytes
        );
      } else {
        if (startIndex + blockCount > _blockCount) {
          blockCount = _blockCount - startIndex;
        }

        for (
          UInt32 blockIndex = 0;
          blockIndex < blockCount;
          ++blockIndex
        ) {
          UInt32 index = startIndex + blockIndex;

          // block should be currently allocated
          if (blockOrders[index] != OrderAllocated) {
            KLOG_TRACE(
              "Kernel block %u at %p is not allocated (order %u)",
              index,
              index * _blockSize,
              blockOrders[index]
            );

            continue;
          }

          // if the block is reference-counted, decrement and only free at zero
          if (_refCounts[index] > 1) {
            --_refCounts[index];

            MemoryBlockTag tag
              = _blockTags
              ? _blockTags[index]
              : MemoryBlockTag::Unknown;

            KLOG_DEBUG(
              "Retained block %u at %p still has %u refs (tag %u)",
              index,
              index * _blockSize,
              _refCounts[index],
              static_cast<UInt32>(tag)
            );

            continue;
          }

          _refCounts[index] = 0;

          if (_usedBlockCount > 0) {
            --_usedBlockCount;
          } else {
            KLOG_WARNING("Used block count is already zero");
          }

          // decrement per-tag count
          if (_blockTags) {
            UInt8 tagIndex = static_cast<UInt8>(_blockTags[index]);

            if (tagIndex < static_cast<UInt8>(MemoryBlockTag::Count)) {
              if (_tagCounts[tagIndex] > 0) {
                --_tagCounts[tagIndex];
              }
            }

            _blockTags[index] = MemoryBlockTag::Unknown;
          }

          // insert into buddy free list and merge
          _buddy.BuddyFree(index);
        }

        KLOG_DEBUG(
          "Freed %u block(s) starting at %u (%p-%p), %u used blocks",
          blockCount,
          startIndex,
          block.Base,
          block.Base + block.SizeInBytes - 1,
          _usedBlockCount
        );
      }
    }
  }

  void IA32MemoryAllocator::Retain(MemoryBlock block) {
    if (block.SizeInBytes > 0) {
      UInt32 startIndex = static_cast<UInt32>(block.Base / _blockSize);
      UInt32 blockCount = static_cast<UInt32>(block.SizeInBytes / _blockSize);

      if (blockCount == 0) {
        blockCount = 1;
      }

      for (
        UInt32 blockIndex = 0;
        blockIndex < blockCount;
        ++blockIndex
      ) {
        UInt32 index = startIndex + blockIndex;

        if (index >= _blockCount) {
          break;
        }

        if (_refCounts[index] == 0) {
          // block was reserved/untracked; start tracking at 2 (one for
          // the original owner, one for the new reference)
          _refCounts[index] = 2;
        } else {
          ++_refCounts[index];
        }
      }

      KLOG_DEBUG(
        "Retained block at %p (%u blocks, new ref count %u)",
        block.Base,
        blockCount,
        _refCounts[startIndex]
      );
    }
  }

  void IA32MemoryAllocator::Reserve(MemoryBlock block) {
    if (block.SizeInBytes > 0) {
      UInt32 start = static_cast<UInt32>(
        AlignDown(block.Base, _blockSize)
      );
      UInt32 end = static_cast<UInt32>(
        AlignUp(
          block.Base + block.SizeInBytes,
          _blockSize
        )
      );
      UInt32 startBlock = start / _blockSize;
      UInt32 endBlock = end / _blockSize;

      if (endBlock > _blockCount) {
        endBlock = _blockCount;
      }

      UInt8* blockOrders = _buddy.GetBlockOrders();

      for (
        UInt32 blockIndex = startBlock;
        blockIndex < endBlock;
        ++blockIndex
      ) {
        if (
          blockOrders[blockIndex] != OrderReserved &&
          blockOrders[blockIndex] != OrderAllocated
        ) {
          blockOrders[blockIndex] = OrderReserved;

          ++_usedBlockCount;
          _reservedBytes += _blockSize;

          if (_blockTags) {
            _blockTags[blockIndex] = MemoryBlockTag::Reserved;
            ++_tagCounts[static_cast<UInt8>(MemoryBlockTag::Reserved)];
          }
        }
      }

      KLOG_DEBUG(
        "Reserved interval [%u, %u], %u used blocks",
        startBlock,
        endBlock,
        _usedBlockCount
      );
    }
  }

  void IA32MemoryAllocator::Release(MemoryBlock block) {
    if (block.SizeInBytes > 0) {
      UInt32 start = static_cast<UInt32>(
        AlignDown(block.Base, _blockSize)
      );
      UInt32 end = static_cast<UInt32>(
        AlignUp(
          block.Base + block.SizeInBytes,
          _blockSize
        )
      );
      UInt32 startBlock = start / _blockSize;
      UInt32 endBlock = end / _blockSize;

      if (endBlock > _blockCount) {
        endBlock = _blockCount;
      }

      UInt8* blockOrders = _buddy.GetBlockOrders();

      for (
        UInt32 blockIndex = startBlock;
        blockIndex < endBlock;
        ++blockIndex
      ) {
        if (blockOrders[blockIndex] == OrderReserved) {
          if (_usedBlockCount > 0) {
            --_usedBlockCount;
          } else {
            KLOG_WARNING("Used block count is already zero");
          }

          _buddy.BuddyFree(blockIndex);
        }
      }

      KLOG_DEBUG(
        "Released kernel block interval [%u, %u], %u used blocks",
        startBlock,
        endBlock,
        _usedBlockCount
      );
    }
  }

  void IA32MemoryAllocator::FreeInitialImage() {
    if (_initialImageInterval.Start < _initialImageInterval.End) {
      Size freedBlockCount = 0;
      UInt8* blockOrders = _buddy.GetBlockOrders();

      for (
        UInt32 blockIndex = _initialImageInterval.Start;
        blockIndex < _initialImageInterval.End;
        ++blockIndex
      ) {
        if (blockOrders[blockIndex] == OrderReserved) {
          if (_blockTags) {
            UInt8 tagIndex = static_cast<UInt8>(_blockTags[blockIndex]);

            if (_tagCounts[tagIndex] > 0) {
              --_tagCounts[tagIndex];
            }

            _blockTags[blockIndex] = MemoryBlockTag::Unknown;
          }

          if (_usedBlockCount > 0) {
            --_usedBlockCount;
          }

          _buddy.BuddyFree(blockIndex);

          ++freedBlockCount;
        }
      }

      Size freedBytes = freedBlockCount * _blockSize;

      if (_reservedBytes >= freedBytes) {
        _reservedBytes -= freedBytes;
      } else {
        _reservedBytes = 0;
      }

      KLOG_INFO("Freed initial image (%u KB)", freedBytes / 1024);

      _initialImageReservedBytes = 0;

      // clear the interval so subsequent calls are no-ops
      _initialImageInterval.Start = 0;
      _initialImageInterval.End = 0;
    }
  }

  UInt32 IA32MemoryAllocator::_calculateManagedBytes(E820BootInfo* bootInfo) {
    KLOG_INFO("bootInfo->EntryCount=%u", bootInfo->EntryCount);

    UInt64 maxUsableAddress = 0;

    if (bootInfo->EntryCount > 0) {
      UInt32 blockFrameIndex = 0;

      for (UInt32 i = 0; i < bootInfo->EntryCount; ++i) {
        const E820Region& region = bootInfo->Entries[i];

        UInt64 baseAddressLog
          = (static_cast<UInt64>(region.BaseHigh) << 32)
          | region.BaseLow;
        UInt64 lengthBytesLog
          = (static_cast<UInt64>(region.LengthHigh) << 32)
          | region.LengthLow;
        UInt64 endAddressLog = baseAddressLog + lengthBytesLog;

        if (lengthBytesLog > 0) {
          endAddressLog -= 1;
        }

        UInt32 endLow = static_cast<UInt32>(endAddressLog & MaxAddress);
        UInt32 endHigh = static_cast<UInt32>(endAddressLog >> 32);

        if (region.BaseHigh == 0 && endHigh == 0) {
          KLOG_INFO(
            "E820[%u]: BaseLow:endLow=%p-%p Type=%u",
            i,
            region.BaseLow,
            endLow,
            region.Type
          );
        } else {
          KLOG_INFO(
            "E820[%u]: BaseHigh:BaseLow=%p:%p "
            "endHigh:endLow=%p:%p Type=%u",
            i,
            region.BaseHigh,
            region.BaseLow,
            endHigh,
            endLow,
            region.Type
          );
        }

        if (region.Type != 1) {
          continue;
        }

        UInt64 baseAddress
          = (static_cast<UInt64>(region.BaseHigh) << 32)
          | region.BaseLow;
        UInt64 lengthBytes
          = (static_cast<UInt64>(region.LengthHigh) << 32)
          | region.LengthLow;

        if (lengthBytes == 0) {
          continue;
        }

        UInt64 endAddress  = baseAddress + lengthBytes;

        if (endAddress < baseAddress || baseAddress > MaxAddress64) {
          continue; // overflow guard
        } else if (endAddress > maxUsableAddress) {
          maxUsableAddress = endAddress;
        }

        constexpr UInt32 blockShift = 12;
        constexpr UInt64 blockMask = (1ull << blockShift) - 1;

        UInt32 startBlock = static_cast<UInt32>(baseAddress >> blockShift);
        UInt32 endBlock = static_cast<UInt32>(
          (endAddress + blockMask) >> blockShift
        );

        if (blockFrameIndex >= _maxBlockFrames) {
          break;
        }

        _blockFrames[blockFrameIndex].Start = startBlock;
        _blockFrames[blockFrameIndex].End = endBlock;
        ++blockFrameIndex;
      }

      _blockFrameCount = blockFrameIndex;
    }

    // cap at 4 GB for IA32
    if (maxUsableAddress > MaxAddress64) {
      maxUsableAddress = MaxAddress64;
    }

    UInt32 managedBytes = static_cast<UInt32>(maxUsableAddress & MaxAddress);

    return managedBytes;
  }

  Interval<UInt32> IA32MemoryAllocator::_calculateInitialImageInterval(
    E820BootInfo* bootInfo
  ) {
    UInt32 startBlock = 0;
    UInt32 endBlock = 0;

    if (
      bootInfo->InitialImagePhysicalAddress != 0 &&
      bootInfo->InitialImageSizeInBytes != 0
    ) {
      UInt32 bundleStartAddress = bootInfo->InitialImagePhysicalAddress;
      UInt32 bundleEndAddress
        = bundleStartAddress
        + bootInfo->InitialImageSizeInBytes - 1;

      startBlock = bundleStartAddress / _blockSize;
      endBlock
        = AlignUp(
            bundleEndAddress + 1,
            _blockSize
          )
        / _blockSize;
    } else {
      startBlock = 0;
      endBlock = 0;
    }

    Interval<UInt32> blockInterval {
      startBlock,
      endBlock
    };

    KLOG_TRACE(
      "imageBlock.Start:imageBlock.End=%u:%u",
      blockInterval.Start,
      blockInterval.End
    );

    return blockInterval;
  }

  void IA32MemoryAllocator::_zero(MemoryBlock block) {
    UInt8* blockBasePtr = reinterpret_cast<UInt8*>(block.Base);

    for (Size i = 0; i < block.SizeInBytes; ++i) {
      blockBasePtr[i] = 0;
    }
  }

  MemoryBlock IA32MemoryAllocator::Allocate(MemoryBlockTag tag) {
    MemoryBlock block = Allocate();

    if (
      block.SizeInBytes > 0 &&
      _blockTags
    ) {
      UInt32 blockIndex = static_cast<UInt32>(block.Base / _blockSize);

      _blockTags[blockIndex] = tag;
      ++_tagCounts[static_cast<UInt8>(tag)];
    }

    return block;
  }

  MemoryBlock IA32MemoryAllocator::Allocate(
    Size size,
    MemoryBlockTag tag
  ) {
    MemoryBlock block = Allocate(size);

    if (
      block.SizeInBytes > 0 &&
      _blockTags
    ) {
      Size blocksAllocated = block.SizeInBytes / _blockSize;
      UInt32 startIndex = static_cast<UInt32>(block.Base / _blockSize);

      for (
        Size blockIndex = 0;
        blockIndex < blocksAllocated;
        ++blockIndex
      ) {
        _blockTags[startIndex + blockIndex] = tag;
        ++_tagCounts[static_cast<UInt8>(tag)];
      }
    }

    return block;
  }

  MemoryBlock IA32MemoryAllocator::AllocateBelow(
    UInt32 limit,
    MemoryBlockTag tag
  ) {
    MemoryBlock block = AllocateBelow(limit);

    if (
      block.SizeInBytes > 0 &&
      _blockTags
    ) {
      UInt32 blockIndex = static_cast<UInt32>(block.Base / _blockSize);

      _blockTags[blockIndex] = tag;
      ++_tagCounts[static_cast<UInt8>(tag)];
    }

    return block;
  }

  Size IA32MemoryAllocator::GetUsedBlockCountByTag(
    MemoryBlockTag tag
  ) const {
    UInt8 tagIndex = static_cast<UInt8>(tag);

    return tagIndex >= static_cast<UInt8>(MemoryBlockTag::Count)
      ? 0
      : _tagCounts[tagIndex];
  }

  void IA32MemoryAllocator::SetOutOfMemoryHandler(
    IOutOfMemoryHandler* handler
  ) {
    _oomHandler = handler;
  }
}
