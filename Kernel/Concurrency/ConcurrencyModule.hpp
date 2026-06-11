/**
 * @file Kernel/Concurrency/ConcurrencyModule.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ConcurrencyModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Modules/IKernelModule.hpp>
#include <Memory/MemoryModule.hpp>
#include <Resources/KernelResourcesModule.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief The concurrency kernel module.
   *
   * Initializes the threading and process management subsystems:
   * @ref Scheduler, @ref ThreadManager, @ref ProcessManager, and
   * @ref FutexManager.
   */
  class ConcurrencyModule : public Core::IKernelModule {
    public:
      static constexpr const char* ID = "Quantum.Kernel.Concurrency";

      Core::KernelModuleID GetID() const override {
        return { ID };
      }

      void GetDependencies(
        Core::KernelModuleID* dependencies,
        Size& count
      ) const override {
        dependencies[0] = { Memory::MemoryModule::ID };
        dependencies[1] = { Resources::KernelResourcesModule::ID };
        count = 2;
      }

      bool Initialize(KernelContext* context) override;
  };
}
