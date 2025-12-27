/**
 * @file System/Kernel/Include/Arch/IA32/IDT.hpp
 * @brief IA32 Interrupt Descriptor Table (IDT) implementation.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Interrupts.hpp>
#include <Prelude.hpp>
#include <Types.hpp>

#include "Interrupts.hpp"

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
        UInt16 limit;

        /**
         * Linear base address of the IDT.
         */
        UInt32 base;
      } __attribute__((packed));

      /**
       * An entry in the IA32 Interrupt Descriptor Table (IDT).
       */
      struct Entry {
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

    private:
      /**
       * Total number of ISR exceptions.
       */
      static constexpr UInt8 _exceptionCount = 32;

      /**
       * IRQ base vector.
       */
      static constexpr UInt8 _irqBase = 32;

      /**
       * Total number of IRQs.
       */
      static constexpr UInt8 _irqCount = 16;

      /**
       * IDT entries array.
       */
      inline static Entry _idtEntries[256];

      /**
       * IDT descriptor for `lidt` instruction.
       */
      inline static Descriptor _idtDescriptor {};

      /**
       * Interrupt handler table.
       */
      inline static Interrupts::Handler _handlerTable[256] = { nullptr };

      /**
       * Exception ISR stub table.
       */
      static void (*const _exceptionStubs[_exceptionCount])();

      /**
       * IRQ ISR stub table.
       */
      static void (*const _irqStubs[_irqCount])();
  };
}
