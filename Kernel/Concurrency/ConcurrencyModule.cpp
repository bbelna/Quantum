/**
 * @file Kernel/Concurrency/ConcurrencyModule.cpp
 * @brief Implements @ref @QKrnl::Concurrency::ConcurrencyModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelContext.hpp>

#include "ConcurrencyModule.hpp"
#include "FutexManager.hpp"
#include "ProcessManager.hpp"
#include "ThreadManager.hpp"

namespace Quantum::Kernel::Concurrency {
  bool ConcurrencyModule::Initialize(KernelContext* context) {
    static HybridCFSBitmapScheduler scheduler;
    static ThreadManager threadManager(
      scheduler,
      *context->Stacks,
      *context->ThreadContextManager,
      *context->CPU,
      *context->MaydayHandler,
      *context->KernelAddressSpace,
      context->UserModeManager
    );
    static FutexManager futexManager(threadManager);
    static ProcessManager processManager(
      threadManager,
      *context->AddressSpaceAllocator,
      *context->MaydayHandler,
      *context->KernelAddressSpace,
      *context->MemoryAllocator,
      *context->MemoryMapper,
      *context->Stacks,
      *context->KernelResourceManager,
      nullptr,
      nullptr,
      context->KernelBase
    );

    context->Scheduler = &scheduler;
    context->ThreadManager = &threadManager;
    context->ProcessesManager = &processManager;
    context->FutexManager = &futexManager;

    return true;
  }
}
