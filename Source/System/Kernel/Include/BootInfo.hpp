/**
 * @file System/Kernel/Include/BootInfo.hpp
 * @brief Architecture-agnostic boot info handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  class BootInfo {
    public:
      /**
       * INIT.BND bundle location provided by the bootloader.
       */
      struct InitBundleInfo {
        UInt32 physical;
        UInt32 size;
      };

      /**
       * Initializes the architecture-specific boot info cache.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

      /**
       * Retrieves INIT.BND bundle info from the cached boot info.
       * @return
       *   True if the bundle is present; false otherwise.
       */
      static bool GetInitBundleInfo(InitBundleInfo& info);
  };
}
