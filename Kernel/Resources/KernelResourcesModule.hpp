/**
 * @file Kernel/Resources/KernelResourcesModule.hpp
 * @brief Declares @ref @QKrnl::Resources::KernelResourcesModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Modules/IKernelModule.hpp>

namespace Quantum::Kernel::Resources {
  /**
   * @brief The resources kernel module.
   *
   * Initializes the @ref KernelResourceManager for tracking per-process
   * resource allocations.
   */
  class KernelResourcesModule : public Core::IKernelModule {
    public:
      static constexpr const char* ID = "Quantum.Kernel.Resources";

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
