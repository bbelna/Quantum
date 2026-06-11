/**
 * @file Runtime/QuantumHeap.cpp
 * @brief Runtime heap allocator implementation for the QuantumOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "QuantumOSRuntimeTypes.hpp"

#include <Quantum/Sync/Mutex.hpp>

namespace {
  using namespace Quantum::Sync;

  /**
   * @brief Mutex protecting the heap free list from concurrent access.
   */
  Mutex HeapLock;

  /**
   * @brief Minimum chunk size for bulk allocation requests (16 pages = 64 KB).
   */
  constexpr UInt32 MinChunkSize = 65536;

  /**
   * @brief Heap block header structure.
   */
  struct HeapBlock {
    /**
     * @brief Size of the block's payload in bytes.
     */
    Size SizeInBytes;

    /**
     * @brief Whether the block is free (`1`) or allocated (`0`).
     */
    UInt32 Free;

    /**
     * @brief Pointer to the next block in the linked list.
     */
    HeapBlock* Next;

    /**
     * @brief Pointer to the previous block in the linked list.
     */
    HeapBlock* Previous;
  };

  /**
   * @brief Head of the heap block linked list.
   */
  HeapBlock* HeapHead = nullptr;

  /**
   * @brief Tail of the heap block linked list.
   */
  HeapBlock* HeapTail = nullptr;

  void SplitBlock(HeapBlock* block, UInt32 sizeInBytes);

  /**
   * @brief Requests a new @ref HeapBlock from the kernel.
   * @param sizeInBytes Desired size of the @ref HeapBlock in bytes.
   * @return Pointer to the new @ref HeapBlock, or `nullptr` on failure.
   *
   * Allocates at least @ref MinChunkSize bytes via the lazy bulk allocation
   * syscall (`Memory_AllocateRange`). Excess space is immediately split
   * into a free block so subsequent allocations can reuse it without
   * additional syscalls.
   */
  HeapBlock* RequestBlock(UInt32 sizeInBytes) {
    UInt32 totalBytes = AlignUp(
      sizeof(HeapBlock) + sizeInBytes,
      8
    );

    if (totalBytes < MinChunkSize) {
      totalBytes = MinChunkSize;
    }

    UInt32 address = Quantum::Kernel::ABI::Invoke(
      KernelOperation::Memory_AllocateRange,
      totalBytes
    );

    if (address > 0) {
      HeapBlock* block = reinterpret_cast<HeapBlock*>(address);

      block->SizeInBytes = totalBytes - sizeof(HeapBlock);
      block->Free = 0;
      block->Next = nullptr;
      block->Previous = HeapTail;

      if (!HeapHead) {
        HeapHead = block;
        HeapTail = block;
      } else {
        HeapTail->Next = block;
        HeapTail = block;
      }

      // split off excess space as a free block
      SplitBlock(
        block,
        sizeInBytes
      );

      return block;
    } else {
      return nullptr;
    }
  }

  /**
   * @brief Splits a @ref HeapBlock if it's significantly larger than the
   *        requested @p sizeInBytes.
   * @param block Pointer to the @ref HeapBlock to split.
   * @param sizeInBytes Desired size in bytes.
   */
  void SplitBlock(
    HeapBlock* block,
    UInt32 sizeInBytes
  ) {
    UInt32 aligned = AlignUp(sizeInBytes, 8);

    if (block->SizeInBytes > aligned + sizeof(HeapBlock) + 8) {
      UInt8* base = reinterpret_cast<UInt8*>(block);
      HeapBlock* next = reinterpret_cast<HeapBlock*>(
        base + sizeof(HeapBlock) + aligned
      );

      next->SizeInBytes = block->SizeInBytes - aligned - sizeof(HeapBlock);
      next->Free = 1;
      next->Next = block->Next;
      next->Previous = block;

      // update the successor's Previous pointer to the new split block
      if (block->Next) {
        block->Next->Previous = next;
      }

      block->SizeInBytes = aligned;
      block->Next = next;

      if (HeapTail == block) {
        HeapTail = next;
      }
    }
  }

  /**
   * @brief Checks whether two @ref HeapBlock are physically adjacent in
   *        memory.
   * @param firstBlock Pointer to the first @ref HeapBlock.
   * @param secondBlock Pointer to the second @ref HeapBlock.
   * @return `true` if the blocks are adjacent, `false` otherwise.
   */
  bool IsAdjacent(
    HeapBlock* firstBlock,
    HeapBlock* secondBlock
  ) {
    UInt8* end
      = reinterpret_cast<UInt8*>(firstBlock)
      + sizeof(HeapBlock)
      + firstBlock->SizeInBytes;

    return end == reinterpret_cast<UInt8*>(secondBlock);
  }

  /**
   * @brief Coalesces a free @ref HeapBlock with adjacent free
   *        @ref HeapBlock in both directions.
   * @param block Pointer to the @ref HeapBlock to coalesce.
   * @return The (possibly changed) coalesced @ref HeapBlock pointer.
   */
  HeapBlock* Coalesce(HeapBlock* block) {
    if (block) {
      // forward: absorb free adjacent successors
      while (
        block->Next &&
        block->Next->Free != 0 &&
        IsAdjacent(block, block->Next)
      ) {
        HeapBlock* next = block->Next;

        block->SizeInBytes += sizeof(HeapBlock) + next->SizeInBytes;
        block->Next = next->Next;

        if (next->Next) {
          next->Next->Previous = block;
        }

        if (HeapTail == next) {
          HeapTail = block;
        }
      }

      // backward: let free adjacent predecessors absorb us
      while (
        block->Previous &&
        block->Previous->Free != 0 &&
        IsAdjacent(block->Previous, block)
      ) {
        HeapBlock* previous = block->Previous;

        previous->SizeInBytes += sizeof(HeapBlock) + block->SizeInBytes;
        previous->Next = block->Next;

        if (block->Next) {
          block->Next->Previous = previous;
        }

        if (HeapTail == block) {
          HeapTail = previous;
        }

        block = previous;
      }

      return block;
    } else {
      return block;
    }
  }
}

/**
 * @brief Allocates @p sizeInBytes of memory.
 * @param sizeInBytes Size to allocate in bytes.
 * @return Opaque `void` pointer to the allocated memory, or `nullptr` on
 *         failure.
 */
void* Allocate(UInt32 sizeInBytes) {
  return malloc(sizeInBytes);
}

/**
 * @brief Allocates @p sizeInBytes of memory.
 * @param sizeInBytes Size to allocate in bytes.
 * @return Opaque `void` pointer to the allocated memory, or `nullptr` on
 *         failure.
 */
extern "C" void* malloc(unsigned int sizeInBytes) {
  if (sizeInBytes == 0) return nullptr;

  HeapLock.Lock();

  UInt32 aligned = AlignUp(sizeInBytes, 8);
  HeapBlock* current = HeapHead;

  while (current) {
    if (current->Free != 0 && current->SizeInBytes >= aligned) {
      current->Free = 0;

      SplitBlock(current, aligned);

      HeapLock.Unlock();

      return current + 1;
    }

    current = current->Next;
  }

  HeapBlock* block = RequestBlock(aligned);

  HeapLock.Unlock();

  if (!block) return nullptr;

  return block + 1;
}

/**
 * @brief Frees previously allocated memory.
 * @param ptr `void` pointer to the memory to free.
 */
void Free(void* ptr) {
  free(ptr);
}

/**
 * @brief Frees previously allocated memory.
 * @param ptr `void` pointer to the memory to free.
 */
extern "C" void free(void* ptr) {
  if (!ptr) return;

  HeapLock.Lock();

  HeapBlock* block = reinterpret_cast<HeapBlock*>(ptr) - 1;

  block->Free = 1;

  // coalesce returns the (possibly different) merged block
  Coalesce(block);

  HeapLock.Unlock();
}

/**
 * @brief C++ global `new` operator.
 * @param size Size to allocate in bytes.
 * @return Opaque `void` pointer to the allocated memory.
 */
void* operator new(unsigned int size) noexcept {
  return malloc(size);
}

/**
 * @brief C++ global `new[]` operator.
 * @param size Size to allocate in bytes.
 * @return Opaque `void` pointer to the allocated memory.
 */
void* operator new[](unsigned int size) noexcept {
  return malloc(size);
}

/**
 * @brief C++ global `delete` operator.
 * @param ptr Opaque `void` pointer to the memory to free.
 */
void operator delete(void* ptr) noexcept {
  free(ptr);
}

/**
 * @brief C++ global `delete[]` operator.
 * @param ptr Opaque `void` pointer to the memory to free.
 */
void operator delete[](void* ptr) noexcept {
  free(ptr);
}

/**
 * @brief C++ global `delete` operator with size. 
 * @param ptr Opaque `void` pointer to the memory to free.
 */
void operator delete(void* ptr, unsigned int /*size*/) noexcept {
  free(ptr);
}

/**
 * @brief C++ global `delete[]` operator with size.
 * @param ptr Opaque `void` pointer to the memory to free.
 */
void operator delete[](void* ptr, unsigned int /*size*/) noexcept {
  free(ptr);
}
