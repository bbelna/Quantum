/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Tests/UserModeTests.hpp
 * User-mode execution tests.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::System::Kernel::Tests {
  /**
   * Registers user-mode related kernel tests.
   */
  class UserModeTests {
    public:
      /**
       * Registers user-mode test cases with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * User virtual base for the test program.
       */
      static constexpr UInt32 _userProgramBase = 0x00400000;

      /**
       * User stack top for the test program.
       */
      static constexpr UInt32 _userStackTop = 0x00800000;

      /**
       * User stack size in bytes for the test program.
       */
      static constexpr UInt32 _userStackSize = 4096;

      /**
       * User-mode test program machine code (yield then exit).
       */
      inline static const UInt8 _userTestProgram[]  = {
        0xB8, 0x02, 0x00, 0x00, 0x00, // mov eax, SYS_YIELD
        0xCD, 0x80,                   // int 0x80
        0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, SYS_EXIT
        0xCD, 0x80,                   // int 0x80
        0xEB, 0xFE                    // jmp $
      };

      /**
       * Verifies user-mode system call and return path.
       * @return
       *   True if the test passes.
       */
      static bool TestUserSyscallPath();
  };
}
