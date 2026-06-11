/**
 * @file Kernel/IPC/IPCModule.cpp
 * @brief Implements @ref @QKrnl::IPC::IPCModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Concurrency/ProcessManager.hpp>
#include <KernelContext.hpp>
#include <Memory/ObjectPool.hpp>
#include <Memory/SharedBufferRepository.hpp>

#include "IPCModule.hpp"
#include "IPCPortRepository.hpp"
#include "IPCPortService.hpp"

namespace Quantum::Kernel::IPC {
  bool IPCModule::Initialize(KernelContext* context) {
    static IPCPortRepository ipcPortRepository(
      *context->HeapAllocator,
      *context->IPCMessagePool,
      *context->IPCPortPool,
      *context->ThreadManager
    );
    static IPCPortService ipcPortService(
      *context->ThreadManager,
      ipcPortRepository,
      *context->HeapAllocator,
      *context->IPCMessagePool,
      *context->MaydayHandler
    );

    context->IPCPortRepository = &ipcPortRepository;
    context->IPCPortService = &ipcPortService;

    static SharedBufferRepository sharedBufferRepository(
      *context->SharedBufferPool
    );

    context->SharedBufferRepository = &sharedBufferRepository;

    // wire up ProcessManager's IPC cleanup dependencies
    context->ProcessesManager->SetIPCDependencies(
      &ipcPortRepository,
      &sharedBufferRepository
    );

    return true;
  }
}
