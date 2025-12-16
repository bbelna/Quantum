/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/IDT.hpp
 * IA32 Interrupt Descriptor Table management.
 */

#pragma once

#include <Arch/IA32/Types/IDT/IDTDescriptor.hpp>
#include <Arch/IA32/Types/IDT/IDTEntry.hpp>
#include <Arch/IA32/Types/IDT/InterruptContext.hpp>
#include <Interrupts.hpp>
#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 Interrupt Descriptor Table manager.
   */
  class IDT {
    public:
      using IDTDescriptor = Types::IDT::IDTDescriptor;
      using IDTEntry = Types::IDT::IDTEntry;
      using InterruptHandler = Quantum::System::Kernel::InterruptHandler;

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
