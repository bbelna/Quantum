/**
 * @file System/Kernel/Tests/UserModeTests.cpp
 * @brief User-mode execution kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Memory.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "Tests/UserModeTests.hpp"

namespace Quantum::System::Kernel::Tests {
  bool UserModeTests::TestUserSyscallPath() {
    Task::DisablePreemption();

    UInt32 addressSpace = Memory::CreateAddressSpace();

    if (addressSpace == 0) {
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to create user address space");

      return false;
    }

    void* codePage = Memory::AllocatePage(true);

    if (codePage == nullptr) {
      Memory::DestroyAddressSpace(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to allocate user program page");

      return false;
    }

    UInt8* codeBytes = static_cast<UInt8*>(codePage);
    constexpr UInt32 stackBytes = _userStackSize;
    constexpr UInt32 stackBase = _userStackTop - stackBytes;
    const UInt32 programSize = static_cast<UInt32>(sizeof(_userTestProgram));

    for (UInt32 i = 0; i < programSize; ++i) {
      codeBytes[i] = _userTestProgram[i];
    }

    Memory::MapPageInAddressSpace(
      addressSpace,
      _userProgramBase,
      reinterpret_cast<UInt32>(codePage),
      false,
      true,
      false
    );

    void* stackPage = Memory::AllocatePage(true);

    if (stackPage == nullptr) {
      Memory::DestroyAddressSpace(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to allocate user stack page");

      return false;
    }

    Memory::MapPageInAddressSpace(
      addressSpace,
      stackBase,
      reinterpret_cast<UInt32>(stackPage),
      true,
      true,
      false
    );

    Task::ControlBlock* tcb = Task::CreateUser(
      _userProgramBase,
      _userStackTop,
      addressSpace
    );

    if (tcb == nullptr) {
      Memory::DestroyAddressSpace(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to create user task");

      return false;
    }

    const UInt32 maxIterations = 128;
    bool terminated = false;

    for (UInt32 i = 0; i < maxIterations; ++i) {
      Task::Yield();

      if (tcb->state == Arch::IA32::Task::State::Terminated) {
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
