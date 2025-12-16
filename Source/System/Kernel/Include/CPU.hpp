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
       * Halts the CPU forever.
       */
      [[noreturn]] static void HaltForever();
  };
}
