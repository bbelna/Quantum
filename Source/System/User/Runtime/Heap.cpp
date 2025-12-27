/**
 * @file System/User/Runtime/Heap.cpp
 * @brief User-mode heap implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <ABI/SystemCall.hpp>
#include <Types.hpp>

namespace {
  /**
   * Heap block header structure.
   */
  struct BlockHeader {
    /**
     * Size of the block's payload in bytes.
     */
    UInt32 size;

    /**
     * Whether the block is free (1) or allocated (0).
     */
    UInt32 free;

    /**
     * Pointer to the next block in the linked list.
     */
    BlockHeader* next;

    /**
     * Padding for alignment.
     */
    UInt32 padding;
  };

  /**
   * Head of the heap block linked list.
   */
  BlockHeader* _heapHead = nullptr;

  /**
   * Tail of the heap block linked list.
   */
  BlockHeader* _heapTail = nullptr;

  /**
   * Aligns a value up to the nearest multiple of align.
   * @param value
   *   The value to align.
   * @param align
   *   The alignment boundary.
   * @return
   *   The aligned value.
   */
  UInt32 AlignUp(UInt32 value, UInt32 align) {
    return (value + align - 1) & ~(align - 1);
  }

  /**
   * Requests a new block from the kernel heap.
   * @param size
   *   Size of the block's payload in bytes.
   * @return
   *   Pointer to the new block header, or nullptr on failure.
   */
  BlockHeader* RequestBlock(UInt32 size) {
    using Quantum::ABI::InvokeSystemCall;
    using Quantum::ABI::SystemCall;

    UInt32 totalBytes = AlignUp(sizeof(BlockHeader) + size, 8);
    UInt32 address = InvokeSystemCall(
      SystemCall::Memory_ExpandHeap,
      totalBytes
    );

    if (address == 0) {
      return nullptr;
    }

    auto* block = reinterpret_cast<BlockHeader*>(address);

    block->size = totalBytes - sizeof(BlockHeader);
    block->free = 0;
    block->next = nullptr;
    block->padding = 0;

    if (!_heapHead) {
      _heapHead = block;
      _heapTail = block;
    } else {
      _heapTail->next = block;
      _heapTail = block;
    }

    return block;
  }

  /**
   * Splits a block if it's significantly larger than the requested size.
   * @param block
   *   Pointer to the block to split.
   * @param size
   *   Size of the requested payload in bytes.
   */
  void SplitBlock(BlockHeader* block, UInt32 size) {
    UInt32 aligned = AlignUp(size, 8);

    if (block->size <= aligned + sizeof(BlockHeader) + 8) {
      return;
    }

    UInt8* base = reinterpret_cast<UInt8*>(block);
    auto* next = reinterpret_cast<BlockHeader*>(
      base + sizeof(BlockHeader) + aligned
    );

    next->size = block->size - aligned - sizeof(BlockHeader);
    next->free = 1;
    next->next = block->next;
    next->padding = 0;

    block->size = aligned;
    block->next = next;

    if (_heapTail == block) {
      _heapTail = next;
    }
  }

  /**
   * Coalesces a free block with its next adjacent free block.
   * @param block
   *   Pointer to the block to coalesce.
   */
  void Coalesce(BlockHeader* block) {
    if (!block || !block->next || block->next->free == 0) {
      return;
    }

    UInt8* end = reinterpret_cast<UInt8*>(block)
      + sizeof(BlockHeader)
      + block->size;

    if (end != reinterpret_cast<UInt8*>(block->next)) {
      return;
    }

    BlockHeader* next = block->next;

    block->size += sizeof(BlockHeader) + next->size;
    block->next = next->next;

    if (_heapTail == next) {
      _heapTail = block;
    }
  }
}

/**
 * Allocates a block of memory of the given size.
 * @param size
 *   Size of the memory block to allocate in bytes.
 * @return
 *   Pointer to the allocated memory block, or `nullptr` on failure.
 */
extern "C" void* malloc(unsigned int size) {
  if (size == 0) {
    return nullptr;
  }

  UInt32 aligned = AlignUp(size, 8);
  BlockHeader* current = _heapHead;

  while (current) {
    if (current->free != 0 && current->size >= aligned) {
      current->free = 0;
      SplitBlock(current, aligned);
      return current + 1;
    }

    current = current->next;
  }

  BlockHeader* block = RequestBlock(aligned);

  if (!block) {
    return nullptr;
  }

  return block + 1;
}

/**
 * Frees a previously allocated block of memory.
 * @param ptr
 *   Pointer to the memory block to free.
 */
extern "C" void free(void* ptr) {
  if (!ptr) {
    return;
  }

  auto* block = reinterpret_cast<BlockHeader*>(ptr) - 1;

  block->free = 1;

  Coalesce(block);
}

/**
 * C++ global `new` operator.
 * @param size
 *   Size of the memory block to allocate in bytes.
 * @return
 *   Pointer to the allocated memory block.
 */
void* operator new(unsigned int size) noexcept {
  return malloc(size);
}

/**
 * C++ global `new[]` operator.
 * @param size
 *   Size of the memory block to allocate in bytes.
 * @return
 *   Pointer to the allocated memory block.
 */
void* operator new[](unsigned int size) noexcept {
  return malloc(size);
}

/**
 * C++ global `delete` operator.
 * @param ptr
 *   Pointer to the memory block to free.
 */
void operator delete(void* ptr) noexcept {
  free(ptr);
}

/**
 * C++ global `delete[]` operator.
 * @param ptr
 *   Pointer to the memory block to free.
 */
void operator delete[](void* ptr) noexcept {
  free(ptr);
}

/**
 * C++ global `delete` operator with size.
 * @param ptr
 *   Pointer to the memory block to free.
 */
void operator delete(void* ptr, unsigned int /*size*/) noexcept {
  free(ptr);
}

/**
 * C++ global `delete[]` operator with size.
 * @param ptr
 *   Pointer to the memory block to free.
 */
void operator delete[](void* ptr, unsigned int /*size*/) noexcept {
  free(ptr);
}
