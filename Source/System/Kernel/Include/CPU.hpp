//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/CPU.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// CPU-related utilities and functions.
//------------------------------------------------------------------------------

#include <Types.hpp>

#pragma once

namespace Quantum::Kernel {
  /**
   * CPU-related utilities and functions.
   */
  class CPU {
    public:
      /**
       * Arch-independent information about the CPU.
       */
      struct CPUInfo {
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
  };
}
