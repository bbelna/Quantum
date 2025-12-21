/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Tests/UserModeTests.hpp
 * User-mode execution tests.
 */

#pragma once

#include <Types.hpp>

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
      static const UInt8 _userTestProgram[];

      /**
       * Verifies user-mode system call and return path.
       * @return
       *   True if the test passes.
       */
      static bool TestUserSyscallPath();
  };
}
