/**
 * @file Kernel/Handlers/HandlersModule.cpp
 * @brief Implements @ref @QKrnl::Handlers::HandlersModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <IPC/IPCPort.hpp>
#include <KernelContext.hpp>
#include <Memory/MemoryPressure.hpp>
#include <Memory/ObjectPool.hpp>
#include <Memory/SharedBuffer.hpp>

#include "HandlersModule.hpp"
#include "OutOfMemoryHandler.hpp"

namespace Quantum::Kernel::Handlers {
  bool HandlersModule::Initialize(KernelContext* context) {
    static MemoryPressureMonitor pressureMonitor(
      *context->MemoryAllocator,
      context->HeapAllocator,
      context->SharedBufferRepository,
      context->IPCMessagePool,
      context->IPCPortPool,
      context->SharedBufferPool
    );

    pressureMonitor.RegisterReclaimer(
      [](MemoryPressure level) -> Size {
        KernelContext* ctx = Context;
        Size freed = 0;

        if (level >= MemoryPressure::Elevated) {
          if (ctx->IPCMessagePool) {
            freed += ctx->IPCMessagePool->Trim();
          }

          if (ctx->IPCPortPool) {
            freed += ctx->IPCPortPool->Trim();
          }

          if (ctx->SharedBufferPool) {
            freed += ctx->SharedBufferPool->Trim();
          }
        }

        return freed;
      }
    );
    pressureMonitor.UpdateState();

    context->MemoryPressureMonitor = &pressureMonitor;

    static OutOfMemoryHandler oomHandler(context);

    context->MemoryAllocator->SetOutOfMemoryHandler(&oomHandler);

    return true;
  }
}
