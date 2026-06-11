/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptManager.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32InterruptManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Interrupts/IInterruptManager.hpp>

#include "IA32IDT.hpp"
#include "IA32InterruptDispatcher.hpp"
#include "IA32InterruptTypes.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief The IA-32 interrupt manager.
   *
   * Owns @ref IA32IDT and @ref IA32InterruptDispatcher. Provides the
   * architecture-independent @ref IInterruptManager interface
   * for the rest of the kernel to register handlers and dispatch
   * exceptions.
   */
  class IA32InterruptManager : public IInterruptManager<IA32InterruptVector> {
    public:
      /**
       * @brief Creates a new @ref IA32InterruptManager.
       * @param interruptControllerDriver
       *   Reference to a concrete implementation of
       *   @ref IInterruptControllerDriver.
       * @param kernelContext Pointer to the @ref KernelContext.
       */
      IA32InterruptManager(
        IInterruptControllerDriver<
          IA32InterruptVector
        >& interruptControllerDriver,
        KernelContext* kernelContext
      );

      /**
       * @brief Destroys the @ref IA32InterruptManager and its owned
       *        resources.
       */
      virtual ~IA32InterruptManager() {
        if (_idt) {
          delete _idt;
        }
      }

      /**
       * @brief Registers an @ref InterruptHandler for the given
       *       @ref IA32InterruptVector.
       * @param vector The @ref IA32InterruptVector to register @p handler to.
       * @param handler The @ref InterruptHandler invoked when the interrupt
       *                occurs.
       */
      void SetHandler(
        IA32InterruptVector vector,
        InterruptHandler handler
      ) override;

      /**
       * @brief Handles an exception interrupt.
       * @param context The interrupt context captured by the ISR stub.
       * @return The interrupt context to restore on `iret`. May differ from
       *         the input if a context switch occurred.
       */
      IA32InterruptContext* HandleException(
        IInterruptContext* context
      ) override;

      /**
       * @brief Gets a pointer to the @ref IA32IDT.
       * @return Pointer to the @ref IA32IDT.
       */
      IA32IDT* IDT() {
        return _idt;
      }

    private:
      /**
       * @brief Pointer to the @ref IA32IDT.
       */
      IA32IDT* _idt = nullptr;

      /**
       * @brief
       *   Reference to a concrete implementation of
       *   @ref IInterruptControllerDriver with an `InterruptVectorType` of
       *   @ref IA32InterruptVector.
       */
      IInterruptControllerDriver<
        IA32InterruptVector
      >& _interruptControllerDriver;

      /**
       * @brief Pointer to the @ref IA32InterruptDispatcher.
       */
      IA32InterruptDispatcher* _dispatcher = nullptr;
  };
}
