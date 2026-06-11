/**
 * @file Kernel/Memory/MemoryModule.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Modules/IKernelModule.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief The memory kernel module.
   *
   * Initializes the @ref StackManager and @ref ObjectPool instances
   * for @ref IPCMessage, @ref IPCPort, and @ref SharedBuffer.
   */
  class MemoryModule : public Core::IKernelModule {
    public:
      static constexpr const char* ID = "Quantum.Kernel.Memory";

      Core::KernelModuleID GetID() const override {
        return { ID };
      }

      void GetDependencies(
        Core::KernelModuleID* dependencies,
        Size& count
      ) const override {
        count = 0;
      }

      bool Initialize(KernelContext* context) override;
  };
}
