/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/CPU.hpp
 * Architecture-agnostic CPU driver.
 */

#pragma once

#include <Types/Primitives.hpp>

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
        char Vendor[32];

        /**
         * The CPU model/description.
         */
        char Model[64];

        /**
         * The number of CPU cores.
         */
        UInt32 CoreCount;

        /**
         * Whether the CPU supports SIMD instructions.
         */
        bool HasSIMD;

        /**
         * Whether the CPU has a hardware FPU.
         */
        bool HasHardwareFPU;

        /**
         * Whether the CPU supports virtualization extensions.
         */
        bool HasVirtualization;
      };

      /**
       * Halts the CPU forever.
       */
      [[noreturn]] static void HaltForever();
  };
}
