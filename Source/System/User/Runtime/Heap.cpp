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
  struct BlockHeader {
    UInt32 size;
    UInt32 free;
    BlockHeader* next;
    UInt32 padding;
  };

  BlockHeader* _heapHead = nullptr;
  BlockHeader* _heapTail = nullptr;

  UInt32 AlignUp(UInt32 value, UInt32 align) {
    return (value + align - 1) & ~(align - 1);
  }

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

extern "C" void free(void* ptr) {
  if (!ptr) {
    return;
  }

  auto* block = reinterpret_cast<BlockHeader*>(ptr) - 1;

  block->free = 1;
  Coalesce(block);
}

void* operator new(unsigned int size) noexcept {
  return malloc(size);
}

void* operator new[](unsigned int size) noexcept {
  return malloc(size);
}

void operator delete(void* ptr) noexcept {
  free(ptr);
}

void operator delete[](void* ptr) noexcept {
  free(ptr);
}

void operator delete(void* ptr, unsigned int /*size*/) noexcept {
  free(ptr);
}

void operator delete[](void* ptr, unsigned int /*size*/) noexcept {
  free(ptr);
}
