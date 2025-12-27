/**
 * @file System/Kernel/Include/CPU.hpp
 * @brief Architecture-agnostic CPU handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * Architecture-agnostic CPU driver.
   */
  class CPU {
    public:
      /**
       * Architecture-agnostic CPU information.
       */
      struct Info {
        /**
         * The CPU's vendor name.
         */
        char vendor[32];

        /**
         * The CPU model/description.
         */
        char model[64];

        /**
         * The number of CPU cores.
         */
        UInt32 coreCount;

        /**
         * Whether the CPU supports SIMD instructions.
         */
        bool hasSIMD;

        /**
         * Whether the CPU has a hardware FPU.
         */
        bool hasHardwareFPU;

        /**
         * Whether the CPU supports virtualization extensions.
         */
        bool hasVirtualization;
      };

      /**
       * Halts the CPU forever.
       */
      [[noreturn]] static void HaltForever();

      /**
       * Pauses the CPU to improve spin-wait loops.
       */
      static void Pause();

      /**
       * Disables CPU interrupts.
       */
      static void DisableInterrupts();

      /**
       * Enables CPU interrupts.
       */
      static void EnableInterrupts();
  };
}
