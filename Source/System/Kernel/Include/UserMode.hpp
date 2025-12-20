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
       * @param userStackTop
       *   Top virtual address of the user stack.
       * @param sizeBytes
       *   Size of the stack in bytes.
       * @return
       *   True on success; false otherwise.
       */
      static bool MapUserStack(UInt32 userStackTop, UInt32 sizeBytes);

      /**
       * Enters user mode at the specified entry point.
       * @param entryPoint
       *   Function pointer to the user-mode entry point.
       * @param userStackTop
       *   Top virtual address of the user stack.
       */
      [[noreturn]] static void Enter(UInt32 entryPoint, UInt32 userStackTop);
  };
}
