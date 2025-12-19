/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/GDT.hpp
 * IA32 Global Descriptor Table (GDT) management.
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
