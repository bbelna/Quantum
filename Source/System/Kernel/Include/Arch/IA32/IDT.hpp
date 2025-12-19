/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/IDT.hpp
 * IA32 Interrupt Descriptor Table management.
 */

#pragma once

#include <Arch/IA32/Interrupts.hpp>
#include <Interrupts.hpp>
#include <Prelude.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 Interrupt Descriptor Table manager.
   */
  class IDT {
    public:
      /**
       * The IDT descriptor structure for the `lidt` instruction.
       */
      struct Descriptor {
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
       * An entry in the IA32 Interrupt Descriptor Table (IDT).
       */
      struct Entry {
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
      static void SetHandler(UInt8 vector, Interrupts::Handler handler);

      /**
       * Sets an IDT gate entry with a specific type attribute.
       * @param vector
       *   The interrupt vector number (0-255).
       * @param stub
       *   ISR stub address.
       * @param typeAttribute
       *   IDT type attribute (e.g., 0x8E for ring0, 0xEE for ring3).
       */
      static void SetGate(UInt8 vector, void (*stub)(), UInt8 typeAttribute);

      /**
       * Dispatches an interrupt to the registered handler.
       * @param context
       *   Interrupt context captured by the stub.
       */
      static Interrupts::Context* DispatchInterrupt(
        Interrupts::Context* context
      );
  };
}
