/**
 * @file System/Kernel/Include/Arch/IA32/GDT.hpp
 * @brief IA32 Global Descriptor Table (GDT) management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 Global Descriptor Table (GDT) management.
   */
  class GDT {
    public:
      /**
       * IA32 GDT entry layout.
       */
      struct [[gnu::packed]] Entry {
        UInt16 limitLow;
        UInt16 baseLow;
        UInt8 baseMid;
        UInt8 access;
        UInt8 granularity;
        UInt8 baseHigh;
      };
  };
}
