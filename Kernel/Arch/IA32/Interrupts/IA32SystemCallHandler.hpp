/**
 * @file Kernel/Arch/IA32/Interrupts/IA32SystemCallHandler.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::SystemCallHandler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IA32InterruptDispatcher.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief IA-32 system call handler.
   * 
   * TODO: All system call logic is currently isolated here in the IA-32 layer.
   * For DR1?, which will add at least one more architecture, this logic needs
   * to be refactored into a common system call handler that the IA-32 and
   * other architecture-specific handlers can delegate to, so that system call
   * implementations are not duplicated across architectures.
   */
  class IA32SystemCallHandler {
    public:
      /**
       * @brief Creates a new @ref IA32SystemCallHandler.
       * @param context Pointer to the @ref KernelContext.
       */
      explicit IA32SystemCallHandler(KernelContext* context);

      /**
       * @brief Destroys this @ref SystemCallHandler instance.
       */
      virtual ~IA32SystemCallHandler() = default;

      /**
       * @brief Handles a system call interrupt.
       * @param context The interrupt context at the time of the system call.
       *        The system call number is in EAX; arguments are in EBX, ECX,
       *        EDX, ESI, EDI.
       * @return The interrupt context to restore on `iret`. The return value
       *         of the system call is placed in EAX. May return a different
       *         context pointer if the system call triggers a context switch.
       */
      IA32InterruptContext* Handle(IA32InterruptContext& context);

    private:
      /**
       * @brief Validates that a user-space pointer range falls within a
       *        mapped region of the given process and is below the kernel
       *        base address.
       * @param process The process whose address space to check.
       * @param address The start of the range to validate.
       * @param sizeInBytes The size of the range in bytes.
       * @return `true` if the range is entirely within a valid user-space
       *         mapping.
       */
      bool _isValidUserPointer(
        Process* process,
        UIntPtr address,
        Size sizeInBytes
      );

      /**
       * @brief Validates a user-space null-terminated string by checking
       *        that at least one byte at the address is within a mapped
       *        user region.
       * @param process The process whose address space to check.
       * @param address The string pointer to validate.
       * @return `true` if the string pointer is in valid user-space
       *         memory.
       */
      bool _isValidUserString(
        Process* process,
        UIntPtr address
      );

      /**
       * @brief Pointer to the kernel context.
       */
      KernelContext* _context = nullptr;
  };
}
