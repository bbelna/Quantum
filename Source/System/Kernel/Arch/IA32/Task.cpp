//------------------------------------------------------------------------------
// Quantum
// System/Kernel/Arch/IA32/Task.cpp
// (c) 2025 Brandon Belna - MIT LIcense
//------------------------------------------------------------------------------
// IA32 task management and context switching implementation.
//------------------------------------------------------------------------------

#include <Arch/IA32/CPU.hpp>
#include <Arch/IA32/Task.hpp>
#include <CPU.hpp>
#include <Kernel.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Types.hpp>

namespace Quantum::Kernel::Arch::IA32 {
  using LogLevel = Logger::Level;

  namespace {
    /**
     * Next task ID to assign.
     */
    UInt32 _nextTaskId = 1;

    /**
     * Pointer to the currently executing task.
     */
    TaskControlBlock* _currentTask = nullptr;

    /**
     * Head of the ready queue (circular linked list).
     */
    TaskControlBlock* _readyQueueHead = nullptr;

    /**
     * Tail of the ready queue.
     */
    TaskControlBlock* _readyQueueTail = nullptr;

    /**
     * Whether preemptive scheduling is enabled.
     */
    bool _preemptionEnabled = false;

    /**
     * Adds a task to the ready queue.
     */
    void AddToReadyQueue(TaskControlBlock* task) {
      task->State = TaskState::Ready;
      task->Next = nullptr;

      if (_readyQueueTail == nullptr) {
        // empty queue
        _readyQueueHead = task;
        _readyQueueTail = task;
      } else {
        // append to tail
        _readyQueueTail->Next = task;
        _readyQueueTail = task;
      }
    }

    /**
     * Removes and returns the next task from the ready queue.
     */
    TaskControlBlock* PopFromReadyQueue() {
      if (_readyQueueHead == nullptr) {
        return nullptr;
      }

      TaskControlBlock* task = _readyQueueHead;
      _readyQueueHead = task->Next;

      if (_readyQueueHead == nullptr) {
        _readyQueueTail = nullptr;
      }

      task->Next = nullptr;
      return task;
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
      // Call the actual task function
      entryPoint();

      // Task returned - terminate it
      Logger::Write(LogLevel::Trace, "Task completed, exiting");
      Task::Exit();
    }

    /**
     * Performs the actual context switch logic and task scheduling.
     */
    void DoSchedule() {
      // if current task is still running, move it back to ready queue
      if (_currentTask != nullptr && _currentTask->State == TaskState::Running) {
        AddToReadyQueue(_currentTask);
      }

      // get next task from ready queue
      TaskControlBlock* nextTask = PopFromReadyQueue();

      if (nextTask == nullptr) {
        // no tasks available - should never happen if idle task exists
        PANIC("No tasks in ready queue");
      }

      // if switching to a different task, perform context switch
      if (nextTask != _currentTask) {
        TaskControlBlock* previousTask = _currentTask;
        _currentTask = nextTask;
        nextTask->State = TaskState::Running;

        if (previousTask != nullptr) {
          // switch from previous to next
          Task::SwitchContext(&previousTask->StackPointer, nextTask->StackPointer);
        } else {
          // first task switch - just load the new context
          Task::SwitchContext(nullptr, nextTask->StackPointer);
        }
      } else {
        // same task - just update state
        _currentTask->State = TaskState::Running;
      }
    }
  }

  void Task::Initialize() {
    Logger::Write(LogLevel::Info, "Creating idle task");

    // create the idle task (runs when nothing else is ready)
    TaskControlBlock* idleTask = Create(IdleTask, 4096);

    if (idleTask == nullptr) {
      PANIC("Failed to create idle task");
    }

    // set it as the current task and start executing
    _currentTask = idleTask;
    idleTask->State = TaskState::Running;

    Logger::Write(LogLevel::Info, "Task subsystem initialized");
  }

  TaskControlBlock* Task::Create(void (*entryPoint)(), UInt32 stackSize) {
    // allocate the task control block
    TaskControlBlock* tcb = static_cast<TaskControlBlock*>(
      Memory::Allocate(sizeof(TaskControlBlock))
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
    tcb->Id = _nextTaskId++;
    tcb->State = TaskState::Ready;
    tcb->StackBase = stack;
    tcb->StackSize = stackSize;
    tcb->Next = nullptr;

    // set up initial stack frame
    // stack grows downward, so place context and call frame below the canary
    const UInt32 canaryBytes = sizeof(UInt32);
    const UInt32 callFrameBytes = sizeof(TaskContext) + 2 * sizeof(UInt32); // dummy return + entry arg

    if (stackSize <= canaryBytes + callFrameBytes) {
      PANIC("Task stack too small");
    }

    UInt8* stackBytes = static_cast<UInt8*>(stack);
    UInt8* frameBase = stackBytes + stackSize - canaryBytes - callFrameBytes;

    TaskContext* context = reinterpret_cast<TaskContext*>(frameBase);
    context->Edi = 0;
    context->Esi = 0;
    context->Ebx = 0;
    context->Ebp = 0;
    context->Eip = reinterpret_cast<UInt32>(&TaskWrapper);

    UInt32* callArea = reinterpret_cast<UInt32*>(context + 1);
    callArea[0] = 0; // dummy return address
    callArea[1] = reinterpret_cast<UInt32>(entryPoint); // argument to TaskWrapper

    // store the stack pointer in the TCB (context is where SwitchContext expects ESP)
    tcb->StackPointer = context;

    // add to ready queue
    AddToReadyQueue(tcb);

    Logger::WriteFormatted(
      LogLevel::Trace,
      "Created task ID=%u, entry=%p, stack=%p-%p",
      tcb->Id,
      entryPoint,
      stack,
      reinterpret_cast<UInt8*>(stack) + stackSize
    );

    return tcb;
  }

  void Task::Exit() {
    Logger::WriteFormatted(
      LogLevel::Trace,
      "Task %u exiting",
      _currentTask->Id
    );

    // mark task as terminated
    _currentTask->State = TaskState::Terminated;

    // free the task's stack
    Memory::Free(_currentTask->StackBase);

    // free the TCB itself
    Memory::Free(_currentTask);

    // schedule next task (never returns)
    _currentTask = nullptr;
    DoSchedule();

    // should never reach here
    PANIC("Exit returned from scheduler");
  }

  void Task::Yield() {
    DoSchedule();
  }

  TaskControlBlock* Task::GetCurrent() {
    return _currentTask;
  }

  void Task::EnablePreemption() {
    _preemptionEnabled = true;
    Logger::Write(LogLevel::Info, "Preemptive multitasking enabled");
  }

  void Task::DisablePreemption() {
    _preemptionEnabled = false;
    Logger::Write(LogLevel::Info, "Preemptive multitasking disabled");
  }

  void Task::Tick() {
    // called from timer interrupt
    if (_preemptionEnabled) {
      DoSchedule();
    }
  }

  // context switch is implemented in assembly
  extern "C" void __attribute__((naked)) SwitchContextAsm(
    TaskContext** currentStackPointer,
    TaskContext* nextStackPointer
  ) {
    asm volatile(
      // save current context if currentStackPointer is not null
      "mov 4(%%esp), %%eax\n"       // eax = currentStackPointer
      "test %%eax, %%eax\n"          // check if null
      "jz 1f\n"                      // skip save if null (first task)

      // save current task's context
      "push %%ebp\n"
      "push %%ebx\n"
      "push %%esi\n"
      "push %%edi\n"

      // store current ESP into *currentStackPointer
      "mov %%esp, (%%eax)\n"

      "1:\n"
      // load next task's context
      "mov 8(%%esp), %%eax\n"        // eax = nextStackPointer
      "mov %%eax, %%esp\n"           // switch to next task's stack

      // restore next task's context
      "pop %%edi\n"
      "pop %%esi\n"
      "pop %%ebx\n"
      "pop %%ebp\n"

      "ret\n"                        // return to next task's EIP
      :::
    );
  }

  void Task::SwitchContext(TaskContext** currentStackPointer, TaskContext* nextStackPointer) {
    SwitchContextAsm(currentStackPointer, nextStackPointer);
  }
}
