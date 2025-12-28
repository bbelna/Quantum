/**
 * @file System/Kernel/Include/Arch/IA32/SystemCalls.hpp
 * @brief IA32 system call handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Interrupts.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 system call handling.
   */
  class SystemCalls {
    public:
      /**
       * System call vector number.
       */
      static constexpr UInt8 vector = 0x80;

      /**
       * Installs the system call gate and handler.
       */
      static void Initialize();

    private:
      /**
       * IA32 system call handler stub.
       */
      static Interrupts::Context* OnSystemCall(Interrupts::Context& context);
  };
}
