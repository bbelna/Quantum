/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/IDT.hpp
 * IA32 Interrupt Descriptor Table management.
 */

#pragma once

#include <Interrupts.hpp>
#include <Prelude.hpp>
#include <Arch/IA32/Types/IDT/IDTDescriptor.hpp>
#include <Arch/IA32/Types/IDT/IDTEntry.hpp>
#include <Arch/IA32/Types/IDT/InterruptContext.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using namespace Types::IDT;

  using Kernel::Types::Interrupts::InterruptHandler;

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
      static InterruptContext* DispatchInterrupt(InterruptContext* context);
  };
}
