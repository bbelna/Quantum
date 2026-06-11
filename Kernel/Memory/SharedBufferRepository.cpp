/**
 * @file Kernel/Memory/SharedBufferRepository.cpp
 * @brief Implements @ref @QKrnl::Memory::SharedBufferRepository.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "KernelLog.hpp"
#include "SharedBufferRepository.hpp"

namespace Quantum::Kernel::Memory {
  SharedBufferRepository::SharedBufferRepository(
    ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>& sharedBufferPool
  ) : _sharedBufferPool(sharedBufferPool) {}

  SharedBuffer* SharedBufferRepository::Create(
    MemoryBlock* blocks,
    Size blockCount,
    Size sizeInBytes
  ) {
    SharedBuffer* buffer = new (_sharedBufferPool.Allocate()) SharedBuffer();

    buffer->ID = _nextID++;
    buffer->Blocks = blocks;
    buffer->BlockCount = blockCount;
    buffer->SizeInBytes = sizeInBytes;
    buffer->AttachCount = 0;

    _buffers.Append(new LinkedNode<SharedBuffer*>(buffer));

    ++_count;

    KLOG_TRACE(
      "Created shared buffer ID %u (%u blocks, %u bytes)",
      buffer->ID,
      blockCount,
      sizeInBytes
    );

    return buffer;
  }

  SharedBuffer* SharedBufferRepository::Find(SharedBufferID id) {
    LinkedNode<SharedBuffer*>* currentBufferNode = _buffers.GetHead();

    while (currentBufferNode) {
      SharedBuffer* buffer = currentBufferNode->GetValue();

      if (buffer && buffer->ID == id) {
        return buffer;
      } else {
        currentBufferNode = currentBufferNode->GetNext();
      }
    }

    return nullptr;
  }

  void SharedBufferRepository::Remove(SharedBufferID id) {
    LinkedNode<SharedBuffer*>* currentBufferNode = _buffers.GetHead();

    while (currentBufferNode) {
      SharedBuffer* buffer = currentBufferNode->GetValue();

      if (
        buffer &&
        buffer->ID == id
      ) {
        _buffers.Remove(currentBufferNode);

        KLOG_TRACE(
          "Removed shared buffer ID %u",
          id
        );

        --_count;

        delete[] buffer->Blocks;

        _sharedBufferPool.Free(buffer);

        delete currentBufferNode;

        return;
      }

      currentBufferNode = currentBufferNode->GetNext();
    }
  }
}
