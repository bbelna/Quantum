/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Tests/MemoryTests.cpp
 * Memory subsystem tests.
 */

#include <Memory.hpp>
#include <Tests.hpp>
#include <Tests/MemoryTests.hpp>

namespace Quantum::System::Kernel {
  namespace {
    /**
     * Verifies basic allocate/free round-trip.
     * @return True if the test passes.
     */
    bool TestMemoryAllocation() {
      void* a = Memory::Allocate(64);
      void* b = Memory::Allocate(128);

      TEST_ASSERT(a != nullptr, "Allocation a returned null");
      TEST_ASSERT(b != nullptr, "Allocation b returned null");

      Memory::Free(a);
      Memory::Free(b);

      return true;
    }
  }

  void MemoryTests::RegisterTests() {
    Tests::Register("Memory alloc/free", TestMemoryAllocation);
  }
}
