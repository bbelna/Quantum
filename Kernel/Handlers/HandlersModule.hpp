/**
 * @file Kernel/Handlers/HandlersModule.hpp
 * @brief Declares @ref @QKrnl::Handlers::HandlersModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/ConcurrencyModule.hpp>
#include <Modules/IKernelModule.hpp>
#include <IPC/IPCModule.hpp>
#include <Memory/MemoryModule.hpp>

namespace Quantum::Kernel::Handlers {
  /**
   * @brief The handlers kernel module.
   *
   * Initializes the @ref MemoryPressureMonitor and registers reclaimers
   * that trim @ref ObjectPool free slabs under pressure. Also initializes
   * the @ref OutOfMemoryHandler.
   */
  class HandlersModule : public Core::IKernelModule {
    public:
      static constexpr const char* ID = "Quantum.Kernel.Handlers";

      Core::KernelModuleID GetID() const override {
        return { ID };
      }

      void GetDependencies(
        Core::KernelModuleID* dependencies,
        Size& count
      ) const override {
        dependencies[0] = { Memory::MemoryModule::ID };
        dependencies[1] = { Concurrency::ConcurrencyModule::ID };
        dependencies[2] = { IPC::IPCModule::ID };
        count = 3;
      }

      bool Initialize(KernelContext* context) override;
  };
}
