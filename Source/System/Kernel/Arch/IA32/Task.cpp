/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Task.cpp
 * IA32 task context and control structures.
 */

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Task.hpp>
#include <CPU.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  using LogLevel = Logger::Level;

  namespace {
    /**
     * Next task ID to assign.
     */
    UInt32 _nextTaskId = 1;

    /**
     * Pointer to the currently executing task.
     */
    Task::ControlBlock* _currentTask = nullptr;

    /**
     * Pointer to the idle task (never exits).
     */
    Task::ControlBlock* _idleTask = nullptr;

    /**
     * Head of the ready queue (circular linked list).
     */
    Task::ControlBlock* _readyQueueHead = nullptr;

    /**
     * Tail of the ready queue.
     */
    Task::ControlBlock* _readyQueueTail = nullptr;

    /**
     * Whether preemptive scheduling is enabled.
     */
    bool _preemptionEnabled = false;

    /**
     * When true, force a reschedule even if preemption is disabled (used by
     * cooperative yields).
     */
    volatile bool _forceReschedule = false;

    /**
     * Task pending cleanup (deferred until we are on a different stack).
     */
    Task::ControlBlock* _pendingCleanup = nullptr;

    /**
     * Becomes true after the first explicit yield; prevents timer interrupts
     * from preempting during early boot before the scheduler is ready.
     */
    bool _schedulerActive = false;

    /**
     * Adds a task to the ready queue.
     */
    void AddToReadyQueue(Task::ControlBlock* task) {
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

    /**
     * Removes and returns the next task from the ready queue.
     * @return
     *   Pointer to the next ready task, or `nullptr` if none are ready.
     */
    Task::ControlBlock* PopFromReadyQueue() {
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

    /**
     * Picks the next task to run and returns its saved context pointer.
     * If `currentContext` is provided, saves it to the current TCB before
     * switching.
     */
    Task::Context* Schedule(Task::Context* currentContext) {
      if (_pendingCleanup && _pendingCleanup != _currentTask) {
        Memory::Free(_pendingCleanup->stackBase);
        Memory::Free(_pendingCleanup);
        _pendingCleanup = nullptr;
      }

      Task::ControlBlock* previousTask = _currentTask;

      if (previousTask != nullptr && currentContext != nullptr) {
        previousTask->context = currentContext;

        if (
          previousTask->state == Task::State::Running &&
          previousTask != _idleTask
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

      if (
        previousTask &&
        previousTask != _idleTask &&
        previousTask->state == Task::State::Terminated &&
        previousTask != nextTask
      ) {
        _pendingCleanup = previousTask;
      }

      return nextTask->context;
    }

    /**
     * Idle task entry point - runs when no other tasks are ready.
     */
    void IdleTask() {
      Logger::Write(LogLevel::Trace, "Idle task running");

      for (;;) {
        CPU::Halt();
      }
    }

    /**
     * Task wrapper that calls the actual entry point and exits cleanly.
     */
    void TaskWrapper(void (*entryPoint)()) {
      // call the actual task function
      entryPoint();

      // task returned - terminate it
      Logger::Write(LogLevel::Debug, "Task completed, exiting");
      Task::Exit();
    }
  }

  void Task::Initialize() {
    _preemptionEnabled = false;
    _forceReschedule = false;
    _pendingCleanup = nullptr;
    _schedulerActive = false;
    _currentTask = nullptr;
    _readyQueueHead = nullptr;
    _readyQueueTail = nullptr;

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
    // allocate the task control block
    Task::ControlBlock* tcb = static_cast<Task::ControlBlock*>(
      Memory::Allocate(sizeof(Task::ControlBlock))
    );

    if (tcb == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate TCB");

      return nullptr;
    }

    // allocate the kernel stack
    void* stack = Memory::Allocate(stackSize);

    if (stack == nullptr) {
      Logger::Write(LogLevel::Error, "Failed to allocate task stack");
      Memory::Free(tcb);

      return nullptr;
    }

    // initialize TCB fields
    tcb->id = _nextTaskId++;
    tcb->state = Task::State::Ready;
    tcb->stackBase = stack;
    tcb->stackSize = stackSize;
    tcb->next = nullptr;

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
    callArea[1] = reinterpret_cast<UInt32>(entryPoint); // TaskWrapper argument

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

    // add to ready queue
    AddToReadyQueue(tcb);

    Logger::WriteFormatted(
      LogLevel::Debug,
      "Created task ID=%u, entry=%p, stack=%p-%p size=%p",
      tcb->id,
      entryPoint,
      stack,
      reinterpret_cast<UInt8*>(stack) + stackSize,
      stackSize
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
}
