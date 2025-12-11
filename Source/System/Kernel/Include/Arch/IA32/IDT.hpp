//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/Arch/IA32/IDT.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// IDT setup for IA32.
//------------------------------------------------------------------------------

#pragma once

#include <Types.hpp>
#include <Interrupts.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using Quantum::Kernel::InterruptHandler;

  /**
   * An entry in the IA32 Interrupt Descriptor Table (IDT).
   */
  struct IDTEntry {
    UInt16 offsetLow;         // bits 0..15 of handler address
    UInt16 selector;          // code segment selector
    UInt8  zero;              // always 0
    UInt8  typeAttribute;     // type and attributes (present, DPL, gate type)
    UInt16 offsetHigh;        // bits 16..31 of handler address
  } __attribute__((packed));

  /**
   * The IDT descriptor structure for the `lidt` instruction.
   */
  struct IDTDescriptor {
    UInt16 limit;
    UInt32 base;
  } __attribute__((packed));

  /**
   * Initializes the IA32 Interrupt Descriptor Table (IDT).
   */
  void InitializeIDT();

  /**
   * Registers a kernel-level interrupt handler for the given vector.
   * @param vector The interrupt vector number (0-255).
   * @param handler The interrupt handler function.
   */
  void SetIDTHandler(UInt8 vector, InterruptHandler handler);
}
