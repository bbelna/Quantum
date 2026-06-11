/**
 * @file Kernel/Arch/IA32/Interrupts/IA32IDT.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32IDT and associated
 *        function stubs.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "IA32IDTDescriptor.hpp"
#include "IA32IDTEntry.hpp"
#include "IA32InterruptContext.hpp"
#include "IA32InterruptConstants.hpp"
#include "IA32InterruptTypes.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  extern "C" {
    /**
    * @brief Assembly stub for the software-initiated scheduler yield
    *        interrupt (vector `49`).
    *
    * Pushes a synthetic error code of zero, saves all registers, and calls
    * into the C++ dispatcher.
    */
    void YIELD49();

    /**
    * @brief System call handler called from the `SYSCALL80` stub.
    */
    void SYSCALL80();

    /**
    * @brief System call handler called from the `SYSENTER` stub.
    */
    void SYSENTER();

    /**
    * @name CPU Exception ISR Stubs (vectors `0`-`31`)
    *
    * Assembly-defined interrupt service routine stubs for the 32 IA-32 CPU
    * exceptions. Each stub pushes the vector number (and a synthetic error
    * code of zero for exceptions that do not push one), saves all
    * general-purpose registers via `pusha`, and jumps to the common C++
    * dispatcher.
    * @{
    */
    void ISR00();
    void ISR01();
    void ISR02();
    void ISR03();
    void ISR04();
    void ISR05();
    void ISR06();
    void ISR07();
    void ISR08();
    void ISR09();
    void ISR10();
    void ISR11();
    void ISR12();
    void ISR13();
    void ISR14();
    void ISR15();
    void ISR16();
    void ISR17();
    void ISR18();
    void ISR19();
    void ISR20();
    void ISR21();
    void ISR22();
    void ISR23();
    void ISR24();
    void ISR25();
    void ISR26();
    void ISR27();
    void ISR28();
    void ISR29();
    void ISR30();
    void ISR31();
    /** @} */

    /**
    * @name Hardware IRQ Stubs (vectors `32`-`47`)
    *
    * Assembly-defined interrupt service routine stubs for the 16 hardware IRQ
    * lines (IRQ `0`-`15`, mapped to vectors `32`-`47` after PIC remapping).
    * Each stub pushes the remapped vector number and a synthetic error code
    * of zero, saves all general-purpose registers, and jumps to the common
    * C++ dispatcher.
    * @{
    */
    void IRQ00();
    void IRQ01();
    void IRQ02();
    void IRQ03();
    void IRQ04();
    void IRQ05();
    void IRQ06();
    void IRQ07();
    void IRQ08();
    void IRQ09();
    void IRQ10();
    void IRQ11();
    void IRQ12();
    void IRQ13();
    void IRQ14();
    void IRQ15();
    /** @} */

    /**
    * @brief Loads the IDT using the `lidt` instruction.
    * @param descriptor Pointer to the @ref IA32IDTDescriptor to load.
    */
    void LoadIDT(IA32IDTDescriptor* descriptor);
  }

  /**
   * @brief Represents the IA-32 Interrupt Descriptor Table (IDT).
   *
   * Owns the 256-entry gate descriptor array loaded via `lidt`, the ISR
   * stub tables for CPU exceptions (vectors `0`-`31`) and hardware IRQs
   * (vectors `32`-`47`), and the per-vector handler function pointer table.
   * The @ref IA32IDT::Dispatch method is called from the common assembly
   * stub to route an interrupt to the registered handler or the
   * @ref IA32InterruptDispatcher.
   */
  class IA32IDT {
    public:
      /**
       * @brief Creates a new @ref InterruptDescriptorTable.
       * @param controller Pointer to an @ref IInterruptControllerDriver.
       */
      explicit IA32IDT(
        IInterruptControllerDriver<IA32InterruptVector>* controller
      );

      /**
       * @brief
       *   Sets the @ref InterruptHandler for the given
       *   @ref IA32InterruptVector.
       * @param vector
       *   The @ref IA32InterruptVector to set the
       *   @ref InterruptHandler for.
       * @param handler
       *   The @ref InterruptHandler to set for the given
       *   @ref IA32InterruptVector.
       */
      void SetHandler(
        IA32InterruptVector vector,
        InterruptHandler handler
      );

      /**
       * @brief Sets the @ref IA32InterruptDispatcher.
       * @param dispatcher Pointer to the @ref IA32InterruptDispatcher to set.
       */
      void SetDispatcher(IA32InterruptDispatcher* dispatcher);

      /**
       * @brief Dispatches an interrupt to the appropriate handler.
       * @param context The @ref IA32InterruptContext.
       * @return
       *   Pointer to the @ref IA32InterruptContext to restore on `iret`. May
       *   differ from the input if a context switch occurred (e.g., scheduler
       *   preemption).
       *
       * For hardware IRQs (vectors `32`-`47`), sends an EOI to the PIC after
       * the handler returns. For the yield vector (`49`), no EOI is sent.
       */
      IA32InterruptContext* Dispatch(IA32InterruptContext* context);

    private:
      /**
       * @brief Pointer to a concrete implementation of @ref
       *        IInterruptControllerDriver with an `InterruptVectorType` of
        *       @ref IA32InterruptVector.
       */
      IInterruptControllerDriver<IA32InterruptVector>* _controller = nullptr;

      /**
       * @brief Pointer to the @ref IA32InterruptDispatcher.
       */
      IA32InterruptDispatcher* _dispatcher = nullptr;

      /**
       * @brief List of @ref IA32IDTEntry structures representing the 256 IDT
       *        entries.
       */
      IA32IDTEntry _entries[IDT_ENTRY_COUNT];

      /**
       * @brief The @ref IA32IDTDescriptor for the `lidt` instruction.
       */
      IA32IDTDescriptor _descriptor = {};

      /**
       * @brief List of @ref InterruptHandler pointers for each
       *        interrupt vector, indexed by vector number.
       * @note Initialized to `nullptr` for all vectors.
       */
      InterruptHandler _handlerTable[IDT_ENTRY_COUNT] = { nullptr };

      /**
       * @brief Exception ISR stub table.
       */
      void (*const _exceptionStubs[ISR_EXCEPTION_COUNT])();

      /**
       * @brief IRQ ISR stub table.
       */
      void (*const _irqStubs[IRQ_COUNT])();

      /**
       * @brief Clears all entries and handlers in the IDT.
       */
      void _clearEntriesAndHandlers();

      /**
       * @brief Sets an entry in the IDT.
       * @param vector The interrupt vector number.
       * @param handler Pointer to the handler function.
       * @param typeAttribute The type and attributes for the entry.
       */
      void _setGate(
        UInt8 vector,
        void (*handler)(),
        UInt8 typeAttribute
      );

      /**
       * @brief Dispatches an @ref IA32InterruptContext to the appropriate
       *        handler.
       * @param context Reference to the @ref IA32InterruptContext to
       *                dispatch.
       * @return Pointer to the next @ref IA32InterruptContext to use.
       */
      IA32InterruptContext* _dispatch(IA32InterruptContext& context);
  };
}
