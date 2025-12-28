/**
 * @file System/Kernel/Tests/MemoryTests.cpp
 * @brief Memory-related kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Heap.hpp"
#include "Testing.hpp"
#include "Tests/MemoryTests.hpp"

namespace Quantum::System::Kernel::Tests {
  bool MemoryTests::TestMemoryAllocation() {
    void* a = Heap::Allocate(64);
    void* b = Heap::Allocate(128);

    TEST_ASSERT(a != nullptr, "Allocation a returned null");
    TEST_ASSERT(b != nullptr, "Allocation b returned null");

    Heap::Free(a);
    Heap::Free(b);

    return true;
  }

  void MemoryTests::RegisterTests() {
    Testing::Register("Memory alloc/free", TestMemoryAllocation);
  }
}
