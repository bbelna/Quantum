/**
 * @file Kernel/Concurrency/ThreadManager.cpp
 * @brief Implements @ref @QKrnl::Concurrency::ThreadManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelLog.hpp>
#include <Memory/StackManager.hpp>
#include <UserMode/IUserModeManager.hpp>

#include "IdleThread.hpp"
#include "Process.hpp"
#include "ThreadManager.hpp"
#include "ThreadWrapper.hpp"

namespace Quantum::Kernel::Concurrency {
  ThreadManager::ThreadManager(
    HybridCFSBitmapScheduler& scheduler,
    StackManager& stackManager,
    IThreadContextManager& contextManager,
    ICPUDriver& cpu,
    IMaydayHandler& maydayHandler,
    IAddressSpace& kernelAddressSpace,
    IUserModeManager* userMode
  ) :
    _scheduler(scheduler),
    _stackManager(stackManager),
    _contextManager(contextManager),
    _cpu(cpu),
    _maydayHandler(maydayHandler),
    _kernelAddressSpace(kernelAddressSpace),
    _userMode(userMode),
    _current(nullptr),
    _idleThread(nullptr),
    _threads {},
    _threadIDs(1),
    _lock(),
    _zombieList(nullptr)
  {
    // create the idle thread
    _idleThread = Create(
      "Idle",
      IdleThread,
      &_cpu,
      ThreadPriority::Idle,
      ThreadFlags::Kernel,
      0
    );


    if (!_idleThread) {
      MAYDAY("Failed to create idle thread");
    }

    // don't add idle thread to the scheduler - it runs when nothing else can
    _idleThread->State = ThreadState::Ready;
  }

  ThreadManager::~ThreadManager() {
    // clean up all threads
    for (
      Size threadIndex = 0;
      threadIndex < MaxThreadCount;
      threadIndex++
    ) {
      if (_threads[threadIndex]) {
        _cleanup(_threads[threadIndex]);

        _threads[threadIndex] = nullptr;
      }
    }
  }

  Thread* ThreadManager::Create(
    const char* name,
    KernelThreadEntryPoint entryPoint,
    void* argument,
    ThreadPriority priority,
    ThreadFlags flags,
    Size stackSize,
    IAddressSpace* addressSpace
  ) {
    if (!entryPoint) {
      KLOG_ERROR("Cannot create thread with null entry point");

      return nullptr;
    }

    _lock.Acquire();

    // allocate TID
    ThreadID id = _allocateID();

    if (
      id == 0 ||
      id >= MaxThreadCount
    ) {
      _lock.Release();

      KLOG_ERROR("Failed to allocate new TID");

      return nullptr;
    }

    // allocate thread structure
    Thread* thread = new Thread();

    if (!thread) {
      _freeID(id);
      _lock.Release();

      KLOG_ERROR(
        "Allocation for TID %u failed",
        id
      );

      return nullptr;
    }

    // initialize per-thread fields (remaining fields use struct defaults)
    thread->ID = id;

    CString::Copy(
      name
        ? name
        : "?",
      thread->Name,
      sizeof(thread->Name)
    );

    thread->BasePriority = priority;
    thread->Flags = flags;
    thread->EntryPoint = entryPoint;
    thread->EntryArgument = argument;

    // determine stack size
    bool isKernelThread = Enum::HasAnyFlag(flags, ThreadFlags::Kernel);
    Size kernelThreadStackSize
      = stackSize > 0
      ? stackSize
      : DefaultKernelThreadStackSize;

    // allocate kernel stack
    thread->KernelStack = _stackManager.Create(
      _kernelAddressSpace,
      kernelThreadStackSize
    );

    if (
      !thread->KernelStack ||
      thread->KernelStack->Base == 0
    ) {
      delete thread;

      _freeID(id);
      _lock.Release();

      KLOG_ERROR(
        "Kernel stack allocation for TID %u failed",
        id
      );

      return nullptr;
    }

    // for user threads, allocate user stack
    if (!isKernelThread) {
      Size userThreadStackSize
        = stackSize > 0
        ? stackSize
        : DefaultUserThreadStackSize;
      IAddressSpace& targetAddressSpace
        = addressSpace
        ? *addressSpace
        : _kernelAddressSpace;

      thread->UserStack = _stackManager.Create(
        targetAddressSpace,
        userThreadStackSize
      );

      if (
        !thread->UserStack ||
        thread->UserStack->Base == 0
      ) {
        _stackManager.Delete(
          _kernelAddressSpace,
          thread->KernelStack
        );

        delete thread;

        _freeID(id);
        _lock.Release();

        KLOG_ERROR(
          "User stack allocation for TID %u failed",
          id
        );

        return nullptr;
      }
    }

    // calculate stack top (stack grows down)
    UIntPtr stackTop
      = thread->KernelStack->Base
      + thread->KernelStack->SizeInBytes;

    // initialize the thread's CPU context
    if (
      !_contextManager.InitializeContext(
        thread,
        reinterpret_cast<UIntPtr>(ThreadWrapper),
        reinterpret_cast<UIntPtr>(argument),
        stackTop,
        isKernelThread
      )
    ) {
      if (thread->UserStack) {
        _stackManager.Delete(
          _kernelAddressSpace,
          thread->UserStack
        );
      }

      _stackManager.Delete(
        _kernelAddressSpace,
        thread->KernelStack
      );

      delete thread;

      _freeID(id);
      _lock.Release();

      KLOG_ERROR("Thread context initialization failed");

      return nullptr;
    }

    // register in thread table
    _threads[id] = thread;

    _lock.Release();

    KLOG_TRACE(
      "Created thread %s with ID %u and stack %p-%p",
      thread->Name,
      thread->ID,
      thread->KernelStack->Base,
      stackTop
    );

    return thread;
  }

  Thread* ThreadManager::CreateUserModeThread(
    const char* name,
    UIntPtr userEntryPoint,
    UIntPtr userStackTop,
    ThreadPriority priority,
    IAddressSpace* addressSpace
  ) {
    if (addressSpace) {
      _lock.Acquire();

      // allocate TID
      ThreadID id = _allocateID();

      if (
        id == 0 ||
        id >= MaxThreadCount
      ) {
        _lock.Release();

        return nullptr;
      }

      // allocate thread structure
      Thread* thread = new Thread();

      if (!thread) {
        _freeID(id);
        _lock.Release();

        return nullptr;
      }

      // initialize per-thread fields (remaining fields use struct defaults)
      thread->ID = id;

      CString::Copy(
        name
          ? name
          : "?",
        thread->Name,
        sizeof(thread->Name)
      );

      thread->BasePriority = priority;
      thread->Flags = ThreadFlags::User | ThreadFlags::Joinable;

      // allocate kernel stack for interrupt handling
      thread->KernelStack = _stackManager.Create(
        _kernelAddressSpace,
        DefaultKernelThreadStackSize
      );

      if (
        !thread->KernelStack ||
        thread->KernelStack->Base == 0
      ) {
        delete thread;

        _freeID(id);
        _lock.Release();

        return nullptr;
      }

      // initialize an empty kernel-mode context on the kernel stack
      UIntPtr kernelStackTop
        = thread->KernelStack->Base
        + thread->KernelStack->SizeInBytes;

      if (
        !_contextManager.InitializeEmptyContext(
          thread,
          kernelStackTop
        )
      ) {
        _stackManager.Delete(
          _kernelAddressSpace,
          thread->KernelStack
        );

        delete thread;

        _freeID(id);
        _lock.Release();

        return nullptr;
      }

      // prepare the thread for user mode execution
      UserModeEntryInfo entryInfo {
        .EntryPoint = userEntryPoint,
        .StackPointer = userStackTop
      };

      if (
        !_userMode->PrepareThread(
          thread,
          entryInfo
        )
      ) {
        _stackManager.Delete(
          _kernelAddressSpace,
          thread->KernelStack
        );

        delete thread;

        _freeID(id);
        _lock.Release();

        return nullptr;
      }

      // register in thread table
      _threads[id] = thread;

      _lock.Release();

      KLOG_TRACE(
        "Created TID %u (%s) with entry at %p",
        thread->ID,
        thread->Name,
        userEntryPoint
      );

      return thread;
    } else {
      KLOG_WARNING("Cannot create thread in null address space");

      return nullptr;
    }
  }

  bool ThreadManager::Start(Thread* thread) {
    if (thread) {
      _lock.Acquire();

      if (thread->State != ThreadState::Created) {
        _lock.Release();

        KLOG_ERROR(
          "Cannot start TID %u (%s): invalid state %d",
          thread->ID,
          thread->Name,
          static_cast<int>(thread->State)
        );

        return false;
      } else {
        thread->State = ThreadState::Ready;

        // initialize CFS vruntime so new threads start at the fairness floor
        _scheduler.InitializeVirtualRuntime(thread);
        _scheduler.Enqueue(thread);

        _lock.Release();

        KLOG_TRACE(
          "Started TID %u (%s)",
          thread->ID,
          thread->Name
        );

        return true;
      }
    } else {
      KLOG_WARNING("Cannot start null thread");

      return false;
    }
  }

  void ThreadManager::TerminateByProcess(
    Process* process,
    Int32 exitCode
  ) {
    if (process) {
      _lock.Acquire();

      for (
        Size threadIndex = 0;
        threadIndex < MaxThreadCount;
        ++threadIndex
      ) {
        Thread* thread = _threads[threadIndex];

        if (
          thread &&
          thread->OwnerProcess == process &&
          thread != _current &&
          thread->State != ThreadState::Terminated
        ) {
          thread->ExitCode = exitCode;
          thread->State = ThreadState::Terminated;

          _scheduler.Remove(thread);

          thread->SchedulerNext = _zombieList;
          _zombieList = thread;

          KLOG_TRACE(
            "Terminated sibling TID %u (%s) of PID %u",
            thread->ID,
            thread->Name,
            process->ID
          );
        }
      }

      _lock.Release();
    } else {
      KLOG_WARNING("Cannot terminate threads for null process");
    }
  }

  void ThreadManager::Terminate(
    Thread* thread,
    Int32 exitCode
  ) {
    if (thread) {
      _lock.Acquire();

      thread->ExitCode = exitCode;
      thread->State = ThreadState::Terminated;

      // remove from scheduler if queued
      _scheduler.Remove(thread);

      // add to zombie list for cleanup
      thread->SchedulerNext = _zombieList;
      _zombieList = thread;

      _lock.Release();

      KLOG_TRACE(
        "Terminated TID %u (%s) and exit code %d",
        thread->ID,
        thread->Name,
        exitCode
      );

      // if terminating current thread, yield to schedule another
      if (thread == _current) {
        Yield();
      }
    } else {
      KLOG_WARNING("Cannot terminate null thread");
    }
  }

  IInterruptContext* ThreadManager::Terminate(
    IInterruptContext* context,
    Int32 exitCode
  ) {
    if (_current) {
      Thread* dying = _current;

      _lock.Acquire();

      dying->ExitCode = exitCode;
      dying->State = ThreadState::Terminated;
      dying->Context = context;

      _scheduler.Remove(dying);

      dying->SchedulerNext = _zombieList;
      _zombieList = dying;

      KLOG_TRACE(
        "TID %u (%s) exited with code %d",
        dying->ID,
        dying->Name,
        exitCode
      );

      // pick the next thread
      Thread* next = _scheduler.Dequeue();

      if (!next) {
        next = _idleThread;
      }

      _lock.Release();

      return _switch(context, next);
    } else {
      KLOG_WARNING("Cannot terminate null thread");

      return context;
    }
  }

  void ThreadManager::Suspend(Thread* thread) {
    if (thread) {
      _scheduler.Suspend(thread);

      if (thread == _current) {
        Yield();
      }
    } else {
      KLOG_WARNING("Cannot suspend null thread");
    }
  }

  void ThreadManager::Resume(Thread* thread) {
    if (thread) {
      _scheduler.Resume(thread);
    } else {
      KLOG_WARNING("Cannot resume null thread");
    }
  }

  void ThreadManager::Sleep(
    Thread* thread,
    UInt64 ticks
  ) {
    if (thread) {
      _scheduler.Sleep(
        thread,
        ticks
      );

      if (thread == _current) {
        Yield();
      }
    } else {
      KLOG_WARNING("Cannot sleep null thread");
    }
  }

  void ThreadManager::Yield() {
    _cpu.Yield();
  }

  IInterruptContext* ThreadManager::Yield(IInterruptContext* context) {
    if (_current) {
      _current->Context = context;

      Thread* next = _scheduler.Yield(_current);

      if (!next) {
        next = _idleThread;
      }

      if (next == _current) {
        _current->State = ThreadState::Running;

        return context;
      } else {
        return _switch(
          context,
          next
        );
      }
    } else {
      return context;
    }
  }

  Thread* ThreadManager::GetCurrent() {
    return _current;
  }

  Thread* ThreadManager::GetByID(ThreadID id) {
    return id >= MaxThreadCount
      ? nullptr
      : _threads[id];
  }

  IInterruptContext* ThreadManager::Tick(IInterruptContext* context) {
    if (_running) {
      _lock.Acquire();

      // move zombies to deferred list (cleaned up after lock release)
      while (_zombieList) {
        Thread* zombie = _zombieList;

        _zombieList = zombie->SchedulerNext;
        zombie->SchedulerNext = _deferredCleanupList;
        _deferredCleanupList = zombie;
      }

      _lock.Release();

      // save current context
      if (_current) {
        _current->Context = context;

        // detect if the ISR context landed on the boot PM stack instead of the
        // thread's kernel stack, this would corrupt the saved state
        if (_current->KernelStack) {
          UInt32 contextAddress = reinterpret_cast<UInt32>(context);
          UInt32 stackBase = _current->KernelStack->Base;
          UInt32 stackTop = stackBase + _current->KernelStack->SizeInBytes;

          if (
            contextAddress < stackBase ||
            contextAddress >= stackTop
          ) {
            KLOG_ERROR(
              "Tick: thread %u (%s) context %p outside stack %p-%p",
              _current->ID,
              _current->Name,
              contextAddress,
              stackBase,
              stackTop
            );

            MAYDAY("ISR context outside thread kernel stack");
          }
        }
      }

      // track idle ticks when the idle thread was running this tick
      if (_current == _idleThread) {
        _scheduler.RecordIdleTick();
      }

      // delegate all scheduling decisions to the Scheduler
      Thread* next = _scheduler.Tick(_current);

      if (!next) {
        next = _idleThread;
      }

      _processDeferredCleanup();

      if (next == _current) {
        _current->State = ThreadState::Running;

        return context;
      } else {
        return _switch(context, next);
      }
    } else {
      return context;
    }
  }

  ThreadID ThreadManager::_allocateID() {
    ThreadID id;

    return !_threadIDs.Allocate(id)
      ? 0
      : id;
  }

  void ThreadManager::_freeID(ThreadID id) {
    _threadIDs.Free(id);
  }

  void ThreadManager::_processDeferredCleanup() {
    while (_deferredCleanupList) {
      Thread* zombie = _deferredCleanupList;

      _deferredCleanupList = zombie->SchedulerNext;

      _cleanup(zombie);
    }
  }

  void ThreadManager::_cleanup(Thread* thread) {
    if (thread) {
      KLOG_TRACE(
        "Cleaning up TID %u (%s), KernelStack=%p, UserStack=%p, "
        "OwnerProcess=%p, AddressSpace=%p",
        thread->ID,
        thread->Name,
        thread->KernelStack
          ? thread->KernelStack->Base
          : 0,
        thread->UserStack
          ? thread->UserStack->Base
          : 0,
        thread->OwnerProcess,
        thread->OwnerProcess
          ? thread->OwnerProcess->AddressSpace
          : nullptr
      );

      // free stacks

      if (thread->KernelStack) {
        _stackManager.Delete(
          _kernelAddressSpace,
          thread->KernelStack
        );
      } else {
        KLOG_WARNING(
          "TID %u has no kernel stack to free",
          thread->ID
        );
      }

      if (thread->UserStack) {
        if (thread->OwnerProcess && thread->OwnerProcess->AddressSpace) {
          _stackManager.Delete(
            *thread->OwnerProcess->AddressSpace,
            thread->UserStack
          );
        } else {
          KLOG_WARNING(
            "TID %u: cannot free user stack - %s is null",
            thread->ID,
            !thread->OwnerProcess
              ? "OwnerProcess"
              : "AddressSpace"
          );
        }
      }

      // update owning process thread count
      if (thread->OwnerProcess && thread->OwnerProcess->ThreadCount > 0) {
        thread->OwnerProcess->ThreadCount--;
      }

      // remove from thread table
      if (thread->ID < MaxThreadCount) {
        _threads[thread->ID] = nullptr;
      }

      _freeID(thread->ID);

      delete thread;
    }
  }

  IInterruptContext* ThreadManager::_switch(
    IInterruptContext* context,
    Thread* next
  ) {
    if (next) {
      Thread* previous = _current;

      _current = next;

      next->State = ThreadState::Running;

      // quantum is already set by Scheduler::Tick() or Scheduler::Yield()
      if (previous) {
        previous->TotalCPUTicks++;
      }

      _scheduler.RecordContextSwitch();

      return _contextManager.SwitchContext(
        context,
        next
      );
    } else {
      return context;
    }
  }
}
