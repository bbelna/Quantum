/**
 * @file Kernel/IPC/IPCModule.hpp
 * @brief Declares @ref @QKrnl::IPC::IPCModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/ConcurrencyModule.hpp>
#include <Modules/IKernelModule.hpp>
#include <Memory/MemoryModule.hpp>

namespace Quantum::Kernel::IPC {
  /**
   * @brief The IPC kernel module.
   *
   * Initializes inter-process communication infrastructure:
   * @ref IPCPortRepository, @ref IPCPortService, and
   * @ref SharedBufferRepository. After initialization, wires up
   * @ref ProcessManager's IPC cleanup dependencies.
   */
  class IPCModule : public Core::IKernelModule {
    public:
      static constexpr const char* ID = "Quantum.Kernel.IPC";

      Core::KernelModuleID GetID() const override {
        return { ID };
      }

      void GetDependencies(
        Core::KernelModuleID* dependencies,
        Size& count
      ) const override {
        dependencies[0] = { Memory::MemoryModule::ID };
        dependencies[1] = { Concurrency::ConcurrencyModule::ID };
        count = 2;
      }

      bool Initialize(KernelContext* context) override;
  };
}
