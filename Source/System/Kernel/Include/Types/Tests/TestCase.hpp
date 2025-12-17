/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Types/Tests/TestCase.hpp
 * Test case descriptor.
 */

#pragma once

#include <Types/Primitives.hpp>
#include <Types/Tests/TestFunction.hpp>

namespace Quantum::System::Kernel::Types::Tests {
  /**
   * Test case descriptor.
   */
  struct TestCase {
    CString Name;
    TestFunction Func;
  };
}
