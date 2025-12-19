/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/UserMode.hpp
 * Kernel user-mode entry helpers.
 */

#pragma once

#include <Types.hpp>

namespace Quantum::System::Kernel {
  class UserMode {
    public:
      /**
       * Maps a user-mode stack at the specified top address.
       * @return
       *   True on success; false otherwise.
       */
      static bool MapUserStack(UInt32 userStackTop, UInt32 sizeBytes);

      /**
       * Enters user mode at the specified entry point.
       */
      [[noreturn]] static void Enter(UInt32 entryPoint, UInt32 userStackTop);
  };
}
