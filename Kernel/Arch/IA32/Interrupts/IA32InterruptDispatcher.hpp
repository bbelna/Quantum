/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptDispatcher.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32InterruptDispatcher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IA32InterruptContext.hpp"
#include "IA32InterruptTypes.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief IA-32 interrupt dispatcher.
   *
   * Routes CPU exceptions (divide-by-zero, invalid opcode, GPF, page fault)
   * to dedicated private handlers and delegates system calls (vector `128`)
   * to the @ref IA32SystemCallHandler. Timer interrupts (vector `32`) and the
   * yield vector (`49`) are forwarded to the scheduler.
   */
  class IA32InterruptDispatcher {
    public:
      /**
       * @brief Creates a new @ref IA32InterruptDispatcher.
       * @param idt Pointer to the @ref IA32IDT.
       * @param kernelContext Pointer to the @ref KernelContext.
       */
      IA32InterruptDispatcher(
        IA32IDT* idt,
        KernelContext* kernelContext
      );

      /**
       * @brief Destroys the @ref IA32InterruptDispatcher.
       */
      virtual ~IA32InterruptDispatcher() = default;

      /**
       * @brief Dispatches an IA-32 interrupt, represented by an
       *        @ref IA32InterruptContext.
       * @param context Reference to the @ref IA32InterruptContext to dispatch.
       * @return Pointer to the @ref IA32InterruptContext to restore on
       *         `iret`.
       */
      IA32InterruptContext* Dispatch(IA32InterruptContext& context);

    private:
      /**
       * @brief Pointer to the @ref IA32IDT.
       */
      IA32IDT* _idt = nullptr;

      /**
       * @brief Pointer to the @ref KernelContext.
       */
      KernelContext* _kernelContext = nullptr;

      /**
       * @brief Pointer to the @ref IA32SystemCallHandler.
       */
      IA32SystemCallHandler* _systemCallHandler = nullptr;

      /**
       * @brief Handler for divide-by-zero faults.
       * @param context Reference to the @ref IA32InterruptContext.
       * @return Pointer to the @ref IA32InterruptContext to restore on `iret`.
       */
      IA32InterruptContext* _divideByZero(IA32InterruptContext& context);

      /**
       * @brief Handler for invalid opcode faults.
       * @param context Reference to the @ref IA32InterruptContext.
       * @return Pointer to the @ref IA32InterruptContext to restore on `iret`.
       */
      IA32InterruptContext* _invalidOpCode(IA32InterruptContext& context);

      /**
       * @brief Handler for general protection faults.
       * @param context Reference to the @ref IA32InterruptContext.
       * @return Pointer to the @ref IA32InterruptContext to restore on `iret`.
       */
      IA32InterruptContext* _generalProtectionFault(
        IA32InterruptContext& context
      );

      /**
       * @brief Handler for page fault exceptions.
       * @param context Reference to the @ref IA32InterruptContext.
       * @return Pointer to the @ref IA32InterruptContext to restore on `iret`.
       */
      IA32InterruptContext* _pageFault(IA32InterruptContext& context);

      /**
       * @brief Logs a detailed register dump and user stack trace for a
       *        user-mode fault (segfault or guard page violation).
       * @param context Reference to the @ref IA32InterruptContext at the time
       *                of the fault.
       * @param process Pointer to the @ref Process that caused the fault.
       * @param faultAddress The address that caused the fault.
       */
      void _logUserFaultContext(
        const IA32InterruptContext& context,
        const Process* process,
        UInt32 faultAddress
      );
  };
}
