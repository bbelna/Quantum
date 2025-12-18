/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/GDT/GDTEntry.hpp
 * IA32 GDT entry structure.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::GDT {
  /**
   * IA32 GDT entry layout.
   */
  struct [[gnu::packed]] GDTEntry {
    UInt16 LimitLow;
    UInt16 BaseLow;
    UInt8 BaseMid;
    UInt8 Access;
    UInt8 Granularity;
    UInt8 BaseHigh;
  };
}
