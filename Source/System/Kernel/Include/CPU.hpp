//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Include/CPU.hpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// CPU-related utilities and functions.
//------------------------------------------------------------------------------

#pragma once

namespace Quantum::Kernel {
  /**
   * CPU-related utilities and functions.
   */
  class CPU {
    public:
      /**
       * Halts the CPU forever.
       */
      [[noreturn]] static void HaltForever();
  };
}
