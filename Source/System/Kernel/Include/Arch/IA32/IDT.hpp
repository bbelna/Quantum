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
    /**
     * Bits 0..15 of handler address.
     */
    UInt16 offsetLow;

    /**
     * Code segment selector.
     */
    UInt16 selector;

    /**
     * Always zero.
     */
    UInt8 zero;

    /**
     * Type and attributes (present, DPL, gate type).
     */
    UInt8 typeAttribute;

    /**
     * Bits 16..31 of handler address.
     */
    UInt16 offsetHigh;
  } __attribute__((packed));

  /**
   * The IDT descriptor structure for the `lidt` instruction.
   */
  struct IDTDescriptor {
    /**
     * Size of the IDT in bytes minus one.
     */
    UInt16 limit;

    /**
     * Linear base address of the IDT.
     */
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
