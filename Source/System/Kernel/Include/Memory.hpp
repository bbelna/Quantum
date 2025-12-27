/**
 * @file System/Kernel/Include/Memory.hpp
 * @brief Kernel memory management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  class Memory {
    public:
      /**
       * Initializes the kernel memory subsystem (paging + allocators).
       * @param bootInfoPhysicalAddress
       *   Physical address of the boot information block.
       */
      static void Initialize(UInt32 bootInfoPhysicalAddress);

  };
}
