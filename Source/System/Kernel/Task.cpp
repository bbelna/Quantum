/**
 * @file System/Kernel/Task.cpp
 * @brief Architecture-agnostic task management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Interrupts.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel {
  using LogLevel = Kernel::Logger::Level;

  void Task::Initialize() {
    Arch::Task::Initialize();
    EnablePreemption();
  }

  Task::ControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    return reinterpret_cast<Task::ControlBlock*>(
      Arch::Task::Create(entryPoint, stackSize)
    );
  }

  Task::ControlBlock* Task::CreateUser(
    UInt32 entryPoint,
    UInt32 userStackTop,
    UInt32 pageDirectoryPhysical
  ) {
    return reinterpret_cast<Task::ControlBlock*>(
      Arch::Task::CreateUser(entryPoint, userStackTop, pageDirectoryPhysical)
    );
  }

  void Task::Exit() {
    Arch::Task::Exit();
  }

  void Task::Yield() {
    Arch::Task::Yield();
  }

  Task::ControlBlock* Task::GetCurrent() {
    return reinterpret_cast<Task::ControlBlock*>(Arch::Task::GetCurrent());
  }

  UInt32 Task::GetCurrentId() {
    auto* tcb = GetCurrent();

    return tcb ? tcb->id : 0;
  }

  void Task::SetCurrentAddressSpace(UInt32 pageDirectoryPhysical) {
    Arch::Task::SetCurrentAddressSpace(pageDirectoryPhysical);
  }

  UInt32 Task::GetCurrentAddressSpace() {
    return Arch::Task::GetCurrentAddressSpace();
  }

  void Task::SetCoordinatorId(UInt32 taskId) {
    _coordinatorTaskId = taskId;
  }

  bool Task::IsCurrentTaskCoordinator() {
    UInt32 current = GetCurrentId();

    return current != 0 && current == _coordinatorTaskId;
  }

  bool Task::GrantIOAccess(UInt32 taskId) {
    return Arch::Task::GrantIOAccess(taskId);
  }

  bool Task::CurrentTaskHasIOAccess() {
    return Arch::Task::CurrentTaskHasIOAccess();
  }

  void Task::EnablePreemption() {
    Arch::Task::EnablePreemption();
  }

  void Task::DisablePreemption() {
    Arch::Task::DisablePreemption();
  }

  Interrupts::Context* Task::Tick(Interrupts::Context& context) {
    return Arch::Task::Tick(context);
  }
}
