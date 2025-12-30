/**
 * @file System/Kernel/Task.cpp
 * @brief Architecture-agnostic task (process) management.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Arch/AddressSpace.hpp"
#include "Arch/Paging.hpp"
#include "Handles.hpp"
#include "Heap.hpp"
#include "Logger.hpp"
#include "Task.hpp"
#include "Thread.hpp"

namespace Quantum::System::Kernel {
  using LogLevel = Logger::Level;

  void Task::Initialize() {
    _coordinatorTaskId = 0;
    _allTasksHead = nullptr;
    _nextTaskId = 1;

    Thread::Initialize();
  }

  Task::ControlBlock* Task::CreateInternal(UInt32 pageDirectoryPhysical) {
    Task::ControlBlock* task = static_cast<Task::ControlBlock*>(
      Heap::Allocate(sizeof(Task::ControlBlock))
    );

    if (task == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate Task");

      return nullptr;
    }

    task->id = _nextTaskId++;
    task->caps = 0;
    task->pageDirectoryPhysical = pageDirectoryPhysical;
    task->userHeapBase = 0;
    task->userHeapEnd = 0;
    task->userHeapMappedEnd = 0;
    task->userHeapLimit = 0;
    task->handleTable = nullptr;
    task->mainThread = nullptr;
    task->next = nullptr;

    HandleTable* handleTable = static_cast<HandleTable*>(
      Heap::Allocate(sizeof(HandleTable))
    );

    if (handleTable != nullptr) {
      handleTable->Initialize();
      task->handleTable = handleTable;
    }

    return task;
  }

  Task::ControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    UInt32 kernelSpace = Arch::Paging::GetKernelPageDirectoryPhysicalAddress();
    Task::ControlBlock* task = CreateInternal(kernelSpace);

    if (task == nullptr) {
      return nullptr;
    }

    Thread::ControlBlock* thread = Thread::Create(task, entryPoint, stackSize);

    if (thread == nullptr) {
      Destroy(task);

      return nullptr;
    }

    task->mainThread = thread;
    AddToAllTasks(task);

    Logger::Write(LogLevel::Debug, "Task created successfully");
    Logger::WriteFormatted(
      LogLevel::Debug,
      "  id=%u entry=%p thread=%u",
      task->id,
      entryPoint,
      thread->id
    );

    return task;
  }

  Task::ControlBlock* Task::CreateUser(
    UInt32 entryPoint,
    UInt32 userStackTop,
    UInt32 pageDirectoryPhysical
  ) {
    if (pageDirectoryPhysical == 0) {
      Logger::Write(LogLevel::Error, "CreateUser: null address space");

      return nullptr;
    }

    Task::ControlBlock* task = CreateInternal(pageDirectoryPhysical);

    if (task == nullptr) {
      return nullptr;
    }

    Thread::ControlBlock* thread = Thread::CreateUser(
      task,
      entryPoint,
      userStackTop
    );

    if (thread == nullptr) {
      Destroy(task);

      return nullptr;
    }

    task->mainThread = thread;
    AddToAllTasks(task);

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Created user task ID=%u entry=%p thread=%u",
      task->id,
      reinterpret_cast<void*>(entryPoint),
      thread->id
    );

    return task;
  }

  void Task::Exit() {
    Thread::Exit();
  }

  void Task::Yield() {
    Thread::Yield();
  }

  Task::ControlBlock* Task::GetCurrent() {
    Thread::ControlBlock* thread = Thread::GetCurrent();

    return thread ? thread->task : nullptr;
  }

  UInt32 Task::GetCurrentId() {
    Task::ControlBlock* task = GetCurrent();

    return task ? task->id : 0;
  }

  void Task::SetCurrentAddressSpace(UInt32 pageDirectoryPhysical) {
    Task::ControlBlock* task = GetCurrent();

    if (task == nullptr) {
      return;
    }

    task->pageDirectoryPhysical = pageDirectoryPhysical;
  }

  UInt32 Task::GetCurrentAddressSpace() {
    Task::ControlBlock* task = GetCurrent();

    return task ? task->pageDirectoryPhysical : 0;
  }

  void Task::SetCoordinatorId(UInt32 taskId) {
    _coordinatorTaskId = taskId;
  }

  bool Task::IsCurrentTaskCoordinator() {
    return _coordinatorTaskId != 0 && _coordinatorTaskId == GetCurrentId();
  }

  bool Task::GrantIOAccess(UInt32 taskId) {
    Task::ControlBlock* task = FindById(taskId);

    if (!task) {
      return false;
    }

    task->caps |= CapabilityIO;

    return true;
  }

  bool Task::CurrentTaskHasIOAccess() {
    Task::ControlBlock* task = GetCurrent();

    return task && (task->caps & CapabilityIO) != 0;
  }

  void Task::EnablePreemption() {
    Thread::EnablePreemption();
  }

  void Task::DisablePreemption() {
    Thread::DisablePreemption();
  }

  Interrupts::Context* Task::Tick(Interrupts::Context& context) {
    return Thread::Tick(context);
  }

  void Task::Destroy(Task::ControlBlock* task) {
    if (task == nullptr) {
      return;
    }

    RemoveFromAllTasks(task);

    if (task->handleTable != nullptr) {
      Heap::Free(task->handleTable);
      task->handleTable = nullptr;
    }

    UInt32 addressSpace = task->pageDirectoryPhysical;

    Heap::Free(task);

    Arch::AddressSpace::Destroy(addressSpace);
  }

  void Task::AddToAllTasks(Task::ControlBlock* task) {
    task->next = _allTasksHead;
    _allTasksHead = task;
  }

  void Task::RemoveFromAllTasks(Task::ControlBlock* task) {
    Task::ControlBlock** current = &_allTasksHead;

    while (*current) {
      if (*current == task) {
        *current = task->next;
        task->next = nullptr;

        return;
      }

      current = &((*current)->next);
    }
  }

  Task::ControlBlock* Task::FindById(UInt32 id) {
    Task::ControlBlock* current = _allTasksHead;

    while (current) {
      if (current->id == id) {
        return current;
      }

      current = current->next;
    }

    return nullptr;
  }
}
