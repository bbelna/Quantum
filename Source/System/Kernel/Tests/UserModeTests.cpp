/**
 * @file System/Kernel/Tests/UserModeTests.cpp
 * @brief User-mode execution kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Arch/AddressSpace.hpp"
#include "Arch/PhysicalAllocator.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "Tests/UserModeTests.hpp"

namespace Quantum::System::Kernel::Tests {
  bool UserModeTests::TestUserSyscallPath() {
    Task::DisablePreemption();

    UInt32 addressSpace = Arch::AddressSpace::Create();

    if (addressSpace == 0) {
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to create user address space");

      return false;
    }

    UInt32 codePage = Arch::PhysicalAllocator::AllocatePage(true);

    if (codePage == 0) {
      Arch::AddressSpace::Destroy(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to allocate user program page");

      return false;
    }

    UInt8* codeBytes = reinterpret_cast<UInt8*>(codePage);
    constexpr UInt32 stackBytes = _userStackSize;
    constexpr UInt32 stackBase = _userStackTop - stackBytes;
    const UInt32 programSize = static_cast<UInt32>(sizeof(_userTestProgram));

    for (UInt32 i = 0; i < programSize; ++i) {
      codeBytes[i] = _userTestProgram[i];
    }

    Arch::AddressSpace::MapPage(
      addressSpace,
      _userProgramBase,
      codePage,
      false,
      true,
      false
    );

    UInt32 stackPage = Arch::PhysicalAllocator::AllocatePage(true);

    if (stackPage == 0) {
      Arch::AddressSpace::Destroy(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to allocate user stack page");

      return false;
    }

    Arch::AddressSpace::MapPage(
      addressSpace,
      stackBase,
      stackPage,
      true,
      true,
      false
    );

    Task::ControlBlock* task = Task::CreateUser(
      _userProgramBase,
      _userStackTop,
      addressSpace
    );

    if (task == nullptr || task->mainThread == nullptr) {
      Arch::AddressSpace::Destroy(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to create user task");

      return false;
    }

    const UInt32 maxIterations = 128;
    bool terminated = false;

    for (UInt32 i = 0; i < maxIterations; ++i) {
      Task::Yield();

      if (task->mainThread->state == Thread::State::Terminated) {
        terminated = true;

        break;
      }
    }

    if (terminated) {
      Task::Yield(); // allow deferred cleanup to run
      Task::EnablePreemption();

      return true;
    }

    Task::EnablePreemption();

    TEST_ASSERT(false, "User task did not terminate");

    return false;
  }

  void UserModeTests::RegisterTests() {
    Testing::Register("User syscall path", TestUserSyscallPath);
  }
}
