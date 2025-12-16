/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/Types/IDT/IDTDescriptor.hpp
 * The IDT descriptor structure for the `lidt` instruction.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32::Types::IDT {
  /**
   * The IDT descriptor structure for the `lidt` instruction.
   */
  struct IDTDescriptor {
    /**
     * Size of the IDT in bytes minus one.
     */
    UInt16 Limit;

    /**
     * Linear base address of the IDT.
     */
    UInt32 Base;
  } __attribute__((packed));
}
