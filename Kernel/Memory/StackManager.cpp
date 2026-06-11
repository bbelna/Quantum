/**
 * @file Kernel/Memory/StackManager.cpp
 * @brief Implements @ref @QKrnl::Memory::StackManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/Spinlock.hpp>
#include <KernelLog.hpp>

#include "StackManager.hpp"

namespace Quantum::Kernel::Memory {
  StackManager::StackManager(
    IMemoryAllocator& memoryAllocator,
    IMemoryMapper& memoryMapper,
    StackManagerConfiguration configuration
  ) :
    _memoryAllocator(memoryAllocator),
    _virtualBlockMapper(memoryMapper),
    _stackBlock(configuration.Block),
    _guardBlockCount(configuration.GuardBlockCount)
  {
    _spinlock = new Concurrency::Spinlock<UInt32>();

    KLOG_TRACE(
      "StackManager initialized with region %p-%p",
      _stackBlock.Base,
      _stackBlock.Base + _stackBlock.SizeInBytes
    );
  }

  Stack* StackManager::Create(
    IAddressSpace& addressSpace,
    Size sizeInBytes,
    MemoryMappingPermissions permissions
  ) {
    // we start by calculating all the stuff we need
    Size stackSizeInBytes = AlignUp(
      sizeInBytes,
      _memoryAllocator.GetBlockSize()
    );
    Size kernelBlockSizeInBytes = _memoryAllocator.GetBlockSize();
    Size guardSizeInBytes = _guardBlockCount * kernelBlockSizeInBytes;
    Size totalBytes = guardSizeInBytes + stackSizeInBytes;
    UIntPtr slotBase {};

    // ok, grab the spinlock for the free list and try to find a slot
    _spinlock->Acquire();

    LinkedNode<MemoryBlock>* previous = nullptr;
    LinkedNode<MemoryBlock>* current = _freeList;

    // search free list for a suitable slot
    while (current != nullptr) {
      if (current->GetValue().SizeInBytes == totalBytes) {
        slotBase = current->GetValue().Base;

        if (previous) {
          previous->SetNext(current->GetNext());
        } else {
          _freeList = current->GetNext();
        }

        delete current;

        _spinlock->Release();

        break;
      }

      previous = current;
      current = current->GetNext();
    }

    // if we didn't find a slot, we need to carve one out of the stack region
    if (slotBase == 0) {
      if (
        _stackBlock.Base + totalBytes >
        _stackBlock.Base + _stackBlock.SizeInBytes
      ) {
        _spinlock->Release();

        KLOG_ERROR("Cannot allocate stack: out of process space");

        return new Stack{};
      }

      slotBase = _stackBlock.Base;
      _stackBlock.Base = _stackBlock.Base + totalBytes;
    }

    // we have a slot, release the spinlock and map it
    _spinlock->Release();

    // create a process address block for the slot
    MemoryBlock slot {
      slotBase,
      totalBytes
    };

    UIntPtr stackBase = slotBase + guardSizeInBytes;
    Stack* stack = new Stack {
      {
        stackBase,
        stackSizeInBytes
      },
      stackBase + stackSizeInBytes
    };

    // map the stack into the address space with the appropriate permissions
    if (!_map(addressSpace, *stack, permissions)) {
      KLOG_ERROR("Failed to allocate stack");

      _releaseSlot(slot);

      delete stack;

      return new Stack();
    }

    // all done!
    return stack;
  }

  void StackManager::Delete(
    IAddressSpace& addressSpace,
    Stack* stack
  ) {
    // sanity checks
    if (!stack || stack->Base == 0) {
      KLOG_ERROR("Cannot free invalid stack");

      return;
    } else if (stack->SizeInBytes == 0) {
      KLOG_ERROR("Cannot free stack at %p with zero size", stack->Base);

      return;
    }

    // calculate the slot for this stack
    Size kernelBlockSizeInBytes = _memoryAllocator.GetBlockSize();
    Size guardSizeInBytes = _guardBlockCount * kernelBlockSizeInBytes;
    MemoryBlock slot {
      stack->Base - guardSizeInBytes,
      stack->SizeInBytes + guardSizeInBytes
    };

    // unmap the stack from the address space
    if (!_unmap(addressSpace, *stack)) {
      KLOG_ERROR("Unmap failed for stack at %p", stack->Base);

      return;
    }

    KLOG_TRACE(
      "Freed stack at %p (%u bytes), slot %p (%u bytes)",
      stack->Base,
      stack->SizeInBytes,
      slot.Base,
      slot.SizeInBytes
    );

    // release the slot back to the free list, and we're done
    _releaseSlot(slot);
  }

  bool StackManager::_map(
    IAddressSpace& addressSpace,
    Stack stack,
    MemoryMappingPermissions permissions
  ) {
    // calculate how many kernel blocks we need to map the stack
    Size kernelBlockSizeInBytes = _memoryAllocator.GetBlockSize();
    Size blocks = stack.SizeInBytes / kernelBlockSizeInBytes;

    // map each block of the stack to a kernel block
    for (Size i = 0; i < blocks; i++) {
      MemoryBlock kernelBlock =
        _memoryAllocator.Allocate(
          kernelBlockSizeInBytes,
          MemoryBlockTag::Stack
        );

      if (kernelBlock.Base == 0) {
        KLOG_ERROR(
          "Allocation failed for stack block at %p",
          stack.Base + (i * kernelBlockSizeInBytes)
        );

        return false;
      }

      // calculate the process address block for this portion of the stack
      MemoryBlock processBlock {
        stack.Base + (i * kernelBlockSizeInBytes),
        kernelBlockSizeInBytes
      };

      // map the process block to the kernel block with the appropriate flags
      if (
        _virtualBlockMapper.Map(
          addressSpace,
          processBlock,
          kernelBlock,
          MemoryMappingFlags {
            MemoryMappingPermissions::Read |
            MemoryMappingPermissions::Write |
            permissions,
            MemoryMappingCache::Default,
            MemoryMappingOptions::Global
          }
        ).Base == 0
      ) {
        KLOG_ERROR(
          "Mapping failed for stack block at %p",
          stack.Base + (i * kernelBlockSizeInBytes)
        );

        return false;
      }
    }

    KLOG_DEBUG(
      "Mapped %u byte stack at %p",
      stack.SizeInBytes,
      stack.Base
    );

    // all done
    return true;
  }

  bool StackManager::_unmap(
    IAddressSpace& addressSpace,
    Stack stack
  ) {
    // calculate how many kernel blocks we need to unmap the stack
    Size kernelBlockSizeInBytes = _memoryAllocator.GetBlockSize();
    Size blocks = stack.SizeInBytes / kernelBlockSizeInBytes;

    // unmap each block of the stack and free the corresponding kernel block
    for (Size i = 0; i < blocks; i++) {
      // calculate the process block for this portion of the stack
      MemoryBlock processBlock {
        stack.Base + (i * kernelBlockSizeInBytes),
        kernelBlockSizeInBytes
      };

      // unmap the process block and free the backing kernel memory
      MemoryBlock kernelBlock
        = _virtualBlockMapper.Unmap(addressSpace, processBlock);

      if (kernelBlock.Base == 0) {
        KLOG_ERROR(
          "Unmapping failed for stack block at %p",
          stack.Base + (i * kernelBlockSizeInBytes)
        );

        return false;
      }

      _memoryAllocator.Free(kernelBlock);

      KLOG_DEBUG(
        "Unmapped stack block at %p (kernel %p)",
        processBlock.Base,
        kernelBlock.Base
      );
    }

    // all done
    return true;
  }

  void StackManager::_releaseSlot(MemoryBlock slot) {
    LinkedNode<MemoryBlock>* node =
      new LinkedNode<MemoryBlock>(slot, _freeList);

    _freeList = node;
  }
}
