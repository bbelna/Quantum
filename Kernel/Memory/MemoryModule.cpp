/**
 * @file Kernel/Memory/MemoryModule.cpp
 * @brief Implements @ref @QKrnl::Memory::MemoryModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelConstants.hpp>
#include <KernelContext.hpp>

#include "MemoryModule.hpp"
#include "ObjectPool.hpp"
#include "StackManager.hpp"

namespace Quantum::Kernel::Memory {
  bool MemoryModule::Initialize(KernelContext* context) {
    static StackManager stackManager(
      *context->MemoryAllocator,
      *context->MemoryMapper,
      context->StackManagerConfigurationProvider
        ->Provide()
    );

    context->Stacks = &stackManager;

    static ObjectPool<IPCMessage, IPC_MESSAGE_POOL_SIZE>
      ipcMessagePool(
        *context->HeapAllocator
      );
    static ObjectPool<IPCPort, IPC_PORT_POOL_SIZE>
      ipcPortPool(
        *context->HeapAllocator
      );
    static ObjectPool<SharedBuffer, SHARED_BUFFER_POOL_SIZE>
      sharedBufferPool(
        *context->HeapAllocator
      );

    context->IPCMessagePool = &ipcMessagePool;
    context->IPCPortPool = &ipcPortPool;
    context->SharedBufferPool = &sharedBufferPool;

    return true;
  }
}
