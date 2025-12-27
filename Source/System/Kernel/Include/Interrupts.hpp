/**
 * @file System/Kernel/Include/Interrupts.hpp
 * @brief Architecture-agnostic interrupt handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "Types.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Interrupts.hpp"
#endif

namespace Quantum::System::Kernel {
  /**
   * Kernel interrupt controller for registering handlers.
   */
  class Interrupts {
    public:
      #if defined(QUANTUM_ARCH_IA32)
      using Context = Arch::IA32::Interrupts::Context;
      #else
      using Context = void;
      #endif

      /**
       * An interrupt handler function.
       * @param context
       *   Reference to the interrupt context.
       * @return
       *   Pointer to the context to resume after handling (can be the same
       *   context).
       */
      using Handler = Context* (*)(Context& context);

      /**
       * Initializes the kernel interrupt subsystem.
       */
      static void Initialize();

      /**
       * Registers an interrupt handler for the given vector.
       * @param vector
       *  The interrupt vector number.
       * @param handler
       *   The interrupt handler function.
       */
      static void RegisterHandler(UInt8 vector, Handler handler);
  };
}
