/**
 * @file System/Kernel/Include/Arch/IA32/MemoryMap.hpp
 * @brief IA32 memory map definitions.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 memory map constants.
   */
  class MemoryMap {
    public:
      /**
       * Base virtual address where the kernel will be mapped in the
       * higher-half. Identity mappings remain available for now to ease the
       * transition.
       */
      static constexpr UInt32 kernelVirtualBase = 0xC0000000;

      /**
       * Page-directory slot reserved for the recursive/self map.
       */
      static constexpr UInt32 recursiveSlot = 1023;

      /**
       * Size of a single page in bytes.
       */
      static constexpr UInt32 pageSize = 4096;

      /**
       * Base virtual address for the kernel heap region.
       */
      static constexpr UInt32 kernelHeapBase = 0xC2000000;

      /**
       * Total virtual bytes reserved for the kernel heap region.
       */
      static constexpr UInt32 kernelHeapBytes = 512 * 1024 * 1024;
  };
}
