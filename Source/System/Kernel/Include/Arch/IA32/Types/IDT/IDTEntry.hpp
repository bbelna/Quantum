/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/IDT/IDTEntry.hpp
 * An entry in the IA32 Interrupt Descriptor Table (IDT).
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::IDT {
  /**
   * An entry in the IA32 Interrupt Descriptor Table (IDT).
   */
  struct IDTEntry {
    /**
     * Bits 0..15 of handler address.
     */
    UInt16 OffsetLow;

    /**
     * Code segment selector.
     */
    UInt16 Selector;

    /**
     * Always zero.
     */
    UInt8 Zero;

    /**
     * Type and attributes (present, DPL, gate type).
     */
    UInt8 TypeAttribute;

    /**
     * Bits 16..31 of handler address.
     */
    UInt16 OffsetHigh;
  } __attribute__((packed));
}
