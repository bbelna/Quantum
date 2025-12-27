/**
 * @file System/Kernel/Arch/IA32/Task.cpp
 * @brief IA32 task context and control structures.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Memory.hpp"
#include "Arch/IA32/Task.hpp"
#include "Arch/IA32/TSS.hpp"
#include "CPU.hpp"
#include "Logger.hpp"
#include "Memory.hpp"
#include "Panic.hpp"
#include "Prelude.hpp"
#include "UserMode.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Kernel::Logger::Level;
  using UserMode = Kernel::UserMode;

  void Task::AddToReadyQueue(Task::ControlBlock* task) {
    task->state = Task::State::Ready;
    task->next = nullptr;

    if (_readyQueueTail == nullptr) {
      // empty queue
      _readyQueueHead = task;
      _readyQueueTail = task;
    } else {
      // append to tail
      _readyQueueTail->next = task;
      _readyQueueTail = task;
    }
  }

  Task::ControlBlock* Task::PopFromReadyQueue() {
    if (_readyQueueHead == nullptr) {
      return nullptr;
    }

    Task::ControlBlock* task = _readyQueueHead;

    _readyQueueHead = task->next;

    if (_readyQueueHead == nullptr) {
      _readyQueueTail = nullptr;
    }

    task->next = nullptr;

    return task;
  }

  void Task::AddToAllTasks(Task::ControlBlock* task) {
    task->allNext = _allTasksHead;
    _allTasksHead = task;
  }

  void Task::RemoveFromAllTasks(Task::ControlBlock* task) {
    Task::ControlBlock** current = &_allTasksHead;

    while (*current) {
      if (*current == task) {
        *current = task->allNext;
        task->allNext = nullptr;

        return;
      }

      current = &((*current)->allNext);
    }
  }

  Task::ControlBlock* Task::FindById(UInt32 id) {
    Task::ControlBlock* current = _allTasksHead;

    while (current) {
      if (current->id == id) {
        return current;
      }

      current = current->allNext;
    }

    return nullptr;
  }

  Task::Context* Task::Schedule(Task::Context* currentContext) {
    if (_pendingCleanup && _pendingCleanup != _currentTask) {
      UInt32 cleanupSpace = _pendingCleanup->pageDirectoryPhysical;

      RemoveFromAllTasks(_pendingCleanup);

      Kernel::Memory::Free(_pendingCleanup->stackBase);
      Kernel::Memory::Free(_pendingCleanup);
      Memory::DestroyAddressSpace(cleanupSpace);

      _pendingCleanup = nullptr;
    }

    Task::ControlBlock* previousTask = _currentTask;

    if (previousTask != nullptr && currentContext != nullptr) {
      previousTask->context = currentContext;

      if (
        previousTask->state == Task::State::Running
        && previousTask != _idleTask
      ) {
        previousTask->state = Task::State::Ready;

        AddToReadyQueue(previousTask);
      }
    }

    Task::ControlBlock* nextTask = PopFromReadyQueue();

    if (nextTask == nullptr) {
      nextTask = _idleTask;
    }

    _currentTask = nextTask;
    nextTask->state = Task::State::Running;

    UInt32 previousSpace
      = previousTask ? previousTask->pageDirectoryPhysical : 0;
    UInt32 nextSpace = nextTask->pageDirectoryPhysical;

    if (nextSpace != 0 && nextSpace != previousSpace) {
      Memory::ActivateAddressSpace(nextSpace);
    }

    if (nextTask->kernelStackTop != 0) {
      TSS::SetKernelStack(nextTask->kernelStackTop);
    }

    if (
      previousTask
      && previousTask != _idleTask
      && previousTask->state == Task::State::Terminated
      && previousTask != nextTask
    ) {
      _pendingCleanup = previousTask;
    }

    return nextTask->context;
  }

  void Task::IdleTask() {
    Logger::Write(LogLevel::Trace, "Idle task running");

    for (;;) {
      CPU::Halt();
    }
  }

  void Task::TaskWrapper(void (*entryPoint)()) {
    // call the actual task function
    entryPoint();

    // task returned - terminate it
    Logger::Write(LogLevel::Debug, "Task completed, exiting");
    Task::Exit();
  }

  void Task::UserTaskTrampoline() {
    Task::ControlBlock* tcb = Task::GetCurrent();

    if (!tcb || tcb->userEntryPoint == 0 || tcb->userStackTop == 0) {
      PANIC("User task missing entry or stack");
    }

    Logger::WriteFormatted(
      LogLevel::Debug,
      "User task %u entry=%p stackTop=%p pageDir=%p",
      tcb->id,
      tcb->userEntryPoint,
      tcb->userStackTop,
      tcb->pageDirectoryPhysical
    );
    Logger::WriteFormatted(
      LogLevel::Debug,
      "User map entry: PDE=%p PTE=%p",
      Memory::GetPageDirectoryEntry(tcb->userEntryPoint),
      Memory::GetPageTableEntry(tcb->userEntryPoint)
    );
    Logger::WriteFormatted(
      LogLevel::Debug,
      "User map stack: PDE=%p PTE=%p",
      Memory::GetPageDirectoryEntry(tcb->userStackTop - 4),
      Memory::GetPageTableEntry(tcb->userStackTop - 4)
    );

    UserMode::Enter(
      tcb->userEntryPoint,
      tcb->userStackTop
    );

    PANIC("User task returned from user mode");
  }

  Task::ControlBlock* Task::CreateTaskInternal(
    void (*entryPoint)(),
    UInt32 stackSize
  ) {
    // allocate the task control block
    Task::ControlBlock* tcb = static_cast<Task::ControlBlock*>(
      Kernel::Memory::Allocate(sizeof(Task::ControlBlock))
    );

    if (tcb == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate TCB");

      return nullptr;
    }

    // allocate the kernel stack
    void* stack = Kernel::Memory::Allocate(stackSize);

    if (stack == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate task stack");
      Kernel::Memory::Free(tcb);

      return nullptr;
    }

    // initialize TCB fields
    tcb->id = _nextTaskId++;
    tcb->caps = 0;
    tcb->pageDirectoryPhysical = Memory::GetKernelPageDirectoryPhysical();
    tcb->state = Task::State::Ready;
    tcb->stackBase = stack;
    tcb->stackSize = stackSize;
    tcb->kernelStackTop = reinterpret_cast<UInt32>(
      static_cast<UInt8*>(stack) + stackSize
    );
    tcb->userEntryPoint = 0;
    tcb->userStackTop = 0;
    tcb->userHeapBase = 0;
    tcb->userHeapEnd = 0;
    tcb->userHeapMappedEnd = 0;
    tcb->userHeapLimit = 0;
    tcb->next = nullptr;
    tcb->allNext = nullptr;

    // ensure stack can hold the bootstrap frame
    const UInt32 minFrame = sizeof(Task::Context) + 8;

    if (stackSize <= minFrame) {
      PANIC("Task stack too small");
    }

    // set up initial stack frame that matches the interrupt context layout
    // stack grows downward; reserve space for a dummy return + entry arg
    UInt8* stackBytes = static_cast<UInt8*>(stack);
    UInt32 userEsp = reinterpret_cast<UInt32>(stackBytes + stackSize - 8);
    UInt32* callArea = reinterpret_cast<UInt32*>(userEsp);

    callArea[0] = 0; // dummy return address
    callArea[1] = reinterpret_cast<UInt32>(entryPoint); // TaskWrapper arg

    // place the saved context below the call frame
    Task::Context* context = reinterpret_cast<Task::Context*>(
      userEsp - sizeof(Task::Context)
    );

    context->edi = 0;
    context->esi = 0;
    context->ebp = 0;
    context->esp = userEsp - 20; // value ESP would have before pusha
    context->ebx = 0;
    context->edx = 0;
    context->ecx = 0;
    context->eax = 0;
    context->vector = 0;
    context->errorCode = 0;
    context->eip = reinterpret_cast<UInt32>(&TaskWrapper);
    context->cs = 0x08;
    context->eflags = 0x202;

    // store the saved context pointer in the TCB
    tcb->context = context;

    return tcb;
  }

  void Task::Initialize() {
    _preemptionEnabled = false;
    _forceReschedule = false;
    _pendingCleanup = nullptr;
    _schedulerActive = false;
    _currentTask = nullptr;
    _readyQueueHead = nullptr;
    _readyQueueTail = nullptr;
    _allTasksHead = nullptr;

    Logger::Write(LogLevel::Debug, "Creating idle task");

    // create the idle task (runs when nothing else is ready)
    Task::ControlBlock* idleTask = Create(IdleTask, 4096);

    if (idleTask == nullptr) {
      PANIC("Failed to create idle task");
    }

    // keep idle task out of the ready queue; it is used as a fallback when
    // nothing else is runnable
    idleTask->state = Task::State::Ready;
    _idleTask = idleTask;

    // remove the idle task from the queue; Create() enqueues by default
    // we want the ready queue to hold only runnable work, with idle as a
    // separate fallback
    _readyQueueHead = nullptr;
    _readyQueueTail = nullptr;

    Logger::Write(LogLevel::Debug, "Idle task created successfully");
  }

  Task::ControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    Task::ControlBlock* tcb = CreateTaskInternal(entryPoint, stackSize);

    if (tcb == nullptr) {
      return nullptr;
    }

    // add to ready queue
    AddToReadyQueue(tcb);
    AddToAllTasks(tcb);

    Logger::Write(LogLevel::Debug, "Task created successfully");
    Logger::WriteFormatted(
      LogLevel::Debug,
      "  id=%u entry=%p stack=%p-%p size=%p",
      tcb->id,
      entryPoint,
      tcb->stackBase,
      reinterpret_cast<UInt8*>(tcb->stackBase) + tcb->stackSize,
      tcb->stackSize
    );

    return tcb;
  }

  Task::ControlBlock* Task::CreateUser(
    UInt32 entryPoint,
    UInt32 userStackTop,
    UInt32 pageDirectoryPhysical,
    UInt32 stackSize
  ) {
    if (pageDirectoryPhysical == 0) {
      Logger::Write(LogLevel::Error, "CreateUser: null address space");

      return nullptr;
    }

    Task::ControlBlock* tcb = CreateTaskInternal(UserTaskTrampoline, stackSize);

    if (tcb == nullptr) {
      return nullptr;
    }

    tcb->pageDirectoryPhysical = pageDirectoryPhysical;
    tcb->userEntryPoint = entryPoint;
    tcb->userStackTop = userStackTop;

    // add to ready queue
    AddToReadyQueue(tcb);
    AddToAllTasks(tcb);

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Created user task ID=%u entry=%p stack=%p-%p size=%p",
      tcb->id,
      reinterpret_cast<void*>(entryPoint),
      tcb->stackBase,
      reinterpret_cast<UInt8*>(tcb->stackBase) + tcb->stackSize,
      tcb->stackSize
    );

    return tcb;
  }

  void Task::Exit() {
    Logger::WriteFormatted(
      LogLevel::Debug,
      "Task %u exiting",
      _currentTask->id
    );

    // mark task as terminated; defer freeing stack/TCB until after next switch
    _currentTask->state = Task::State::Terminated;
    _schedulerActive = true;
    _forceReschedule = true;

    asm volatile("int $32" ::: "memory");

    // should never reach here
    PANIC("Exit returned from scheduler");

    __builtin_unreachable();
  }

  void Task::Yield() {
    _schedulerActive = true;
    _forceReschedule = true;

    asm volatile("int $32" ::: "memory");
  }

  Task::ControlBlock* Task::GetCurrent() {
    return _currentTask;
  }

  void Task::SetCurrentAddressSpace(UInt32 pageDirectoryPhysical) {
    if (_currentTask == nullptr) {
      return;
    }

    _currentTask->pageDirectoryPhysical = pageDirectoryPhysical;
  }

  UInt32 Task::GetCurrentAddressSpace() {
    return _currentTask ? _currentTask->pageDirectoryPhysical : 0;
  }

  void Task::EnablePreemption() {
    _preemptionEnabled = true;

    Logger::Write(LogLevel::Debug, "Preemptive multitasking enabled");
  }

  void Task::DisablePreemption() {
    _preemptionEnabled = false;

    Logger::Write(LogLevel::Debug, "Preemptive multitasking disabled");
  }

  Task::Context* Task::Tick(Task::Context& context) {
    // called from timer interrupt
    bool shouldSchedule
      = (_preemptionEnabled && _schedulerActive) || _forceReschedule;

    _forceReschedule = false;

    if (!shouldSchedule) {
      return &context;
    }

    return Schedule(&context);
  }

  bool Task::GrantIOAccess(UInt32 taskId) {
    Task::ControlBlock* tcb = FindById(taskId);

    if (!tcb) {
      return false;
    }

    tcb->caps |= CapabilityIO;

    return true;
  }

  bool Task::CurrentTaskHasIOAccess() {
    Task::ControlBlock* tcb = GetCurrent();

    return tcb && (tcb->caps & CapabilityIO) != 0;
  }
}
