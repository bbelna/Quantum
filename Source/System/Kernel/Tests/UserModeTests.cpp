/**
 * @file System/Kernel/Tests/UserModeTests.cpp
 * @brief User-mode execution kernel tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "AddressSpace.hpp"
#include "PhysicalAllocator.hpp"
#include "Task.hpp"
#include "Testing.hpp"
#include "Tests/UserModeTests.hpp"

namespace Quantum::System::Kernel::Tests {
  bool UserModeTests::TestUserSyscallPath() {
    Task::DisablePreemption();

    UInt32 addressSpace = AddressSpace::Create();

    if (addressSpace == 0) {
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to create user address space");

      return false;
    }

    void* codePage = PhysicalAllocator::AllocatePage(true);

    if (codePage == nullptr) {
      AddressSpace::Destroy(addressSpace);
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

    AddressSpace::MapPageInAddressSpace(
      addressSpace,
      _userProgramBase,
      reinterpret_cast<UInt32>(codePage),
      false,
      true,
      false
    );

    void* stackPage = PhysicalAllocator::AllocatePage(true);

    if (stackPage == nullptr) {
      AddressSpace::Destroy(addressSpace);
      Task::EnablePreemption();

      TEST_ASSERT(false, "Failed to allocate user stack page");

      return false;
    }

    AddressSpace::MapPageInAddressSpace(
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
      AddressSpace::Destroy(addressSpace);
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
