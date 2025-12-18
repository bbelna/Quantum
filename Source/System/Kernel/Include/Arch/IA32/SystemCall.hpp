/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Arch/IA32/SystemCall.hpp
 * IA32 system call initialization.
 */

#pragma once

#include <Types/Primitives.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  /**
   * IA32 system call initialization.
   */
  class SystemCall {
    public:
      /**
       * System call vector number.
       */
      static constexpr UInt8 Vector = 0x80;

      /**
       * Installs the system call gate and handler.
       */
      static void Initialize();
  };
}
