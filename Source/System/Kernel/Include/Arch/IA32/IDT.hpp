/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/IDT.hpp
 * IA32 Interrupt Descriptor Table management.
 */

#pragma once

#include <Interrupts.hpp>
#include <Types/Primitives.hpp>
#include <Arch/IA32/InterruptContext.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using Quantum::Kernel::InterruptHandler;

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

  /**
   * IA32 Interrupt Descriptor Table manager.
   */
  class IDT {
    public:
      /**
       * Initializes the IA32 Interrupt Descriptor Table (IDT).
       */
      static void Initialize();

      /**
       * Registers a kernel-level interrupt handler for the given vector.
       * @param vector
       *   The interrupt vector number (0-255).
       * @param handler
       *   The interrupt handler function.
       */
      static void SetHandler(UInt8 vector, InterruptHandler handler);

      /**
       * Dispatches an interrupt to the registered handler.
       * @param context
       *   Interrupt context captured by the stub.
       */
      static void DispatchInterrupt(InterruptContext* context);
  };
}
