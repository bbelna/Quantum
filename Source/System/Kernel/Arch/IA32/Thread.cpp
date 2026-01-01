/**
 * @file System/Kernel/Arch/IA32/Thread.cpp
 * @brief IA32 thread context and control structures.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Arch/IA32/AddressSpace.hpp"
#include "Arch/IA32/CPU.hpp"
#include "Arch/IA32/Paging.hpp"
#include "Arch/IA32/Thread.hpp"
#include "Arch/IA32/TSS.hpp"
#include "CPU.hpp"
#include "Heap.hpp"
#include "Logger.hpp"
#include "Panic.hpp"
#include "Prelude.hpp"
#include "Task.hpp"
#include "UserMode.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using Kernel::Heap;
  using Kernel::Task;
  using Kernel::TaskControlBlock;
  using Kernel::UserMode;

  using LogLevel = Kernel::Logger::Level;

  void Thread::AddToReadyQueue(Thread::ControlBlock* thread) {
    thread->state = Thread::State::Ready;
    thread->next = nullptr;

    if (_readyQueueTail == nullptr) {
      // empty queue
      _readyQueueHead = thread;
      _readyQueueTail = thread;
    } else {
      // append to tail
      _readyQueueTail->next = thread;
      _readyQueueTail = thread;
    }
  }

  Thread::ControlBlock* Thread::PopFromReadyQueue() {
    if (_readyQueueHead == nullptr) {
      return nullptr;
    }

    Thread::ControlBlock* thread = _readyQueueHead;

    _readyQueueHead = thread->next;

    if (_readyQueueHead == nullptr) {
      _readyQueueTail = nullptr;
    }

    thread->next = nullptr;

    return thread;
  }

  void Thread::AddToAllThreads(Thread::ControlBlock* thread) {
    thread->allNext = _allThreadsHead;
    _allThreadsHead = thread;
  }

  void Thread::RemoveFromAllThreads(Thread::ControlBlock* thread) {
    Thread::ControlBlock** current = &_allThreadsHead;

    while (*current) {
      if (*current == thread) {
        *current = thread->allNext;
        thread->allNext = nullptr;

        return;
      }

      current = &((*current)->allNext);
    }
  }

  Thread::ControlBlock* Thread::FindById(UInt32 id) {
    Thread::ControlBlock* current = _allThreadsHead;

    while (current) {
      if (current->id == id) {
        return current;
      }

      current = current->allNext;
    }

    return nullptr;
  }

  static void RemoveFromTaskList(
    TaskControlBlock* task,
    Thread::ControlBlock* thread
  ) {
    if (task == nullptr || thread == nullptr) {
      return;
    }

    Thread::ControlBlock** current = &task->threadHead;

    while (*current) {
      if (*current == thread) {
        *current = thread->taskNext;
        thread->taskNext = nullptr;

        if (task->threadCount > 0) {
          task->threadCount -= 1;
        }

        return;
      }

      current = &((*current)->taskNext);
    }
  }

  Thread::Context* Thread::Schedule(Thread::Context* currentContext) {
    if (_pendingCleanup && _pendingCleanup != _currentThread) {
      TaskControlBlock* cleanupTask = _pendingCleanup->task;

      RemoveFromAllThreads(_pendingCleanup);
      RemoveFromTaskList(cleanupTask, _pendingCleanup);

      Heap::Free(_pendingCleanup->stackBase);
      Heap::Free(_pendingCleanup);

      if (cleanupTask && cleanupTask->threadCount == 0) {
        Task::Destroy(cleanupTask);
      }

      _pendingCleanup = nullptr;
    }

    Thread::ControlBlock* previousThread = _currentThread;

    if (previousThread != nullptr && currentContext != nullptr) {
      previousThread->context = currentContext;

      if (
        previousThread->state == Thread::State::Running
        && previousThread != _idleThread
      ) {
        previousThread->state = Thread::State::Ready;

        AddToReadyQueue(previousThread);
      }
    }

    Thread::ControlBlock* nextThread = PopFromReadyQueue();

    if (nextThread == nullptr) {
      nextThread = _idleThread;
    }

    _currentThread = nextThread;
    nextThread->state = Thread::State::Running;

    TaskControlBlock* previousTask
      = previousThread ? previousThread->task : nullptr;
    TaskControlBlock* nextTask = nextThread->task;
    UInt32 previousSpace = previousTask
      ? previousTask->pageDirectoryPhysical
      : 0;
    UInt32 nextSpace = nextTask ? nextTask->pageDirectoryPhysical : 0;

    if (nextSpace != 0 && nextSpace != previousSpace) {
      AddressSpace::Activate(nextSpace);
    }

    if (nextThread->kernelStackTop != 0) {
      TSS::SetKernelStack(nextThread->kernelStackTop);
    }

    if (
      previousThread
      && previousThread != _idleThread
      && previousThread->state == Thread::State::Terminated
      && previousThread != nextThread
    ) {
      _pendingCleanup = previousThread;
    }

    return nextThread->context;
  }

  void Thread::IdleThread() {
    Logger::Write(LogLevel::Trace, "Idle thread running");

    for (;;) {
      CPU::Halt();
    }
  }

  void Thread::ThreadWrapper(void (*entryPoint)()) {
    // call the actual thread function
    entryPoint();

    // thread returned - terminate it
    Logger::Write(LogLevel::Debug, "Thread completed, exiting");
    Thread::Exit();
  }

  void Thread::UserThreadTrampoline() {
    Thread::ControlBlock* tcb = Thread::GetCurrent();
    TaskControlBlock* task = tcb ? tcb->task : nullptr;

    if (!tcb || !task || tcb->userEntryPoint == 0 || tcb->userStackTop == 0) {
      PANIC("User thread missing entry or stack");
    }

    Logger::WriteFormatted(
      LogLevel::Debug,
      "User thread %u entry=%p stackTop=%p pageDir=%p",
      tcb->id,
      tcb->userEntryPoint,
      tcb->userStackTop,
      task->pageDirectoryPhysical
    );
    Logger::WriteFormatted(
      LogLevel::Debug,
      "User map entry: PDE=%p PTE=%p",
      Paging::GetPageDirectoryEntry(tcb->userEntryPoint),
      Paging::GetPageTableEntry(tcb->userEntryPoint)
    );
    Logger::WriteFormatted(
      LogLevel::Debug,
      "User map stack: PDE=%p PTE=%p",
      Paging::GetPageDirectoryEntry(tcb->userStackTop - 4),
      Paging::GetPageTableEntry(tcb->userStackTop - 4)
    );

    UserMode::Enter(
      tcb->userEntryPoint,
      tcb->userStackTop
    );

    PANIC("User thread returned from user mode");
  }

  Thread::ControlBlock* Thread::CreateThreadInternal(
    TaskControlBlock* task,
    void (*entryPoint)(),
    UInt32 stackSize
  ) {
    if (task == nullptr) {
      Logger::Write(LogLevel::Error, "CreateThreadInternal: missing task");

      return nullptr;
    }

    // allocate the thread control block
    Thread::ControlBlock* tcb = static_cast<Thread::ControlBlock*>(
      Heap::Allocate(sizeof(Thread::ControlBlock))
    );

    if (tcb == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate TCB");

      return nullptr;
    }

    // allocate the kernel stack
    void* stack = Heap::Allocate(stackSize);

    if (stack == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate thread stack");
      Heap::Free(tcb);

      return nullptr;
    }

    // initialize TCB fields
    tcb->id = _nextThreadId++;
    tcb->task = task;
    tcb->state = Thread::State::Ready;
    tcb->stackBase = stack;
    tcb->stackSize = stackSize;
    tcb->kernelStackTop = reinterpret_cast<UInt32>(
      static_cast<UInt8*>(stack) + stackSize
    );
    tcb->userEntryPoint = 0;
    tcb->userStackTop = 0;
    tcb->taskNext = nullptr;
    tcb->next = nullptr;
    tcb->allNext = nullptr;
    tcb->waitNext = nullptr;

    // ensure stack can hold the bootstrap frame
    const UInt32 minFrame = sizeof(Thread::Context) + 8;

    if (stackSize <= minFrame) {
      PANIC("Thread stack too small");
    }

    // set up initial stack frame that matches the interrupt context layout
    // stack grows downward; reserve space for a dummy return + entry arg
    UInt8* stackBytes = static_cast<UInt8*>(stack);
    UInt32 userEsp = reinterpret_cast<UInt32>(stackBytes + stackSize - 8);
    UInt32* callArea = reinterpret_cast<UInt32*>(userEsp);

    callArea[0] = 0; // dummy return address
    callArea[1] = reinterpret_cast<UInt32>(entryPoint); // ThreadWrapper arg

    // place the saved context below the call frame
    Thread::Context* context = reinterpret_cast<Thread::Context*>(
      userEsp - sizeof(Thread::Context)
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
    context->eip = reinterpret_cast<UInt32>(&ThreadWrapper);
    context->cs = 0x08;
    context->eflags = 0x202;

    // store the saved context pointer in the TCB
    tcb->context = context;

    return tcb;
  }

  void Thread::Initialize() {
    _preemptionEnabled = false;
    _forceReschedule = false;
    _pendingCleanup = nullptr;
    _schedulerActive = false;
    _preemptDisableCount = 0;
    _nextThreadId = 1;
    _currentThread = nullptr;
    _readyQueueHead = nullptr;
    _readyQueueTail = nullptr;
    _allThreadsHead = nullptr;

    Logger::Write(LogLevel::Debug, "Creating idle thread");

    // create the idle thread (runs when nothing else is ready)
    TaskControlBlock* idleTask = Task::Create(IdleThread, 4096);

    if (idleTask == nullptr || idleTask->mainThread == nullptr) {
      PANIC("Failed to create idle thread");
    }

    Thread::ControlBlock* idleThread = idleTask->mainThread;

    // keep idle thread out of the ready queue; it is used as a fallback when
    // nothing else is runnable
    idleThread->state = Thread::State::Ready;
    _idleThread = idleThread;

    // remove the idle thread from the queue; Create() enqueues by default
    // we want the ready queue to hold only runnable work, with idle as a
    // separate fallback
    _readyQueueHead = nullptr;
    _readyQueueTail = nullptr;

    Logger::Write(LogLevel::Debug, "Idle thread created successfully");
  }

  Thread::ControlBlock* Thread::Create(
    TaskControlBlock* task,
    void (*entryPoint)(),
    UInt32 stackSize
  ) {
    Thread::ControlBlock* tcb = CreateThreadInternal(
      task,
      entryPoint,
      stackSize
    );

    if (tcb == nullptr) {
      return nullptr;
    }

    // add to ready queue
    AddToReadyQueue(tcb);
    AddToAllThreads(tcb);

    if (task->mainThread == nullptr) {
      task->mainThread = tcb;
    }

    tcb->taskNext = task->threadHead;
    task->threadHead = tcb;
    task->threadCount += 1;

    Logger::Write(LogLevel::Debug, "Thread created successfully");
    Logger::WriteFormatted(
      LogLevel::Debug,
      "  id=%u entry=%p stack=%p-%p size=%p task=%u",
      tcb->id,
      entryPoint,
      tcb->stackBase,
      reinterpret_cast<UInt8*>(tcb->stackBase) + tcb->stackSize,
      tcb->stackSize,
      task ? task->id : 0
    );

    return tcb;
  }

  Thread::ControlBlock* Thread::CreateUser(
    TaskControlBlock* task,
    UInt32 entryPoint,
    UInt32 userStackTop,
    UInt32 stackSize
  ) {
    if (task == nullptr) {
      Logger::Write(LogLevel::Error, "CreateUser: missing task");

      return nullptr;
    }

    Thread::ControlBlock* tcb = CreateThreadInternal(
      task,
      UserThreadTrampoline,
      stackSize
    );

    if (tcb == nullptr) {
      return nullptr;
    }

    tcb->userEntryPoint = entryPoint;
    tcb->userStackTop = userStackTop;

    // add to ready queue
    AddToReadyQueue(tcb);
    AddToAllThreads(tcb);

    if (task->mainThread == nullptr) {
      task->mainThread = tcb;
    }

    tcb->taskNext = task->threadHead;
    task->threadHead = tcb;
    task->threadCount += 1;

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Created user thread ID=%u entry=%p stack=%p-%p size=%p task=%u",
      tcb->id,
      reinterpret_cast<void*>(entryPoint),
      tcb->stackBase,
      reinterpret_cast<UInt8*>(tcb->stackBase) + tcb->stackSize,
      tcb->stackSize,
      task ? task->id : 0
    );

    return tcb;
  }

  void Thread::Exit() {
    Logger::WriteFormatted(
      LogLevel::Debug,
      "Thread %u exiting",
      _currentThread ? _currentThread->id : 0
    );

    // mark thread as terminated; defer freeing stack/TCB until after next
    // switch
    if (_currentThread != nullptr) {
      _currentThread->state = Thread::State::Terminated;
    }

    _schedulerActive = true;
    _forceReschedule = true;

    asm volatile("int $32" ::: "memory");

    // should never reach here
    PANIC("Exit returned from scheduler");

    __builtin_unreachable();
  }

  void Thread::Yield() {
    _schedulerActive = true;
    _forceReschedule = true;

    asm volatile("int $32" ::: "memory");
  }

  Thread::ControlBlock* Thread::GetCurrent() {
    return _currentThread;
  }

  void Thread::EnablePreemption() {
    if (_preemptDisableCount > 0) {
      _preemptDisableCount -= 1;
    }

    if (_preemptDisableCount == 0 && !_preemptionEnabled) {
      _preemptionEnabled = true;
      _schedulerActive = true;

      Logger::Write(LogLevel::Debug, "Preemptive multitasking enabled");
    }
  }

  void Thread::DisablePreemption() {
    _preemptDisableCount += 1;

    if (_preemptionEnabled) {
      _preemptionEnabled = false;

      Logger::Write(LogLevel::Debug, "Preemptive multitasking disabled");
    }
  }

  Thread::Context* Thread::Tick(Thread::Context& context) {
    // called from timer interrupt
    bool preemptionAllowed
      = _preemptionEnabled && _preemptDisableCount == 0;
    bool shouldSchedule
      = (preemptionAllowed && _schedulerActive) || _forceReschedule;

    _forceReschedule = false;

    if (!shouldSchedule) {
      return &context;
    }

    return Schedule(&context);
  }

  void Thread::Wake(Thread::ControlBlock* thread) {
    if (thread == nullptr || thread->state != Thread::State::Blocked) {
      return;
    }

    AddToReadyQueue(thread);
  }
}
