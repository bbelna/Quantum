/**
 * @file Kernel/Concurrency/ThreadManager.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ThreadManager.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "IThreadContextManager.hpp"
#include "HybridCFSBitmapScheduler.hpp"
#include "Spinlock.hpp"
#include "Thread.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Manages @ref Thread lifecycle and context switching.
   *
   * Owns the global thread table (up to @ref MaxThreadCount entries) and
   * drives context switches through @ref IThreadContextManager. All
   * scheduling decisions (run queues, sleep queue, quantum management,
   * CFS vruntime accounting) are delegated to the @ref Scheduler.
   *
   * Public methods like @ref Tick, @ref Yield, @ref Sleep, @ref Suspend,
   * and @ref Resume are thin wrappers that delegate to the Scheduler and
   * handle the resulting context switch.
   *
   * All public methods are internally synchronized via a spinlock. The
   * @ref Tick method is called from the timer interrupt handler and must
   * not be called from normal kernel code.
   */
  class ThreadManager {
    public:
      /**
       * @brief Creates a new @ref ThreadManager instance.
       * @param scheduler The scheduler for scheduling decisions.
       * @param stackManager The stack manager for thread stacks.
       * @param contextManager The arch-specific context manager.
       * @param cpu Reference to the CPU driver.
       * @param maydayHandler Reference to the kernel mayday handler.
       * @param kernelAddressSpace Reference to the kernel address space.
       * @param userMode Pointer to the user mode manager (may be null).
       */
      explicit ThreadManager(
        HybridCFSBitmapScheduler& scheduler,
        StackManager& stackManager,
        IThreadContextManager& contextManager,
        ICPUDriver& cpu,
        IMaydayHandler& maydayHandler,
        IAddressSpace& kernelAddressSpace,
        IUserModeManager* userMode
      );

      /**
       * @brief Destroys this @ref ThreadManager instance.
       */
      ~ThreadManager();

      /**
       * @brief Creates a new kernel-mode @ref Thread.
       * @param name Human-readable @ref Thread name.
       * @param entryPoint @ref Thread entry function.
       * @param argument Argument passed to @p entryPoint on first run.
       * @param priority @ref Thread scheduling priority (`0`-`31`).
       * @param flags @ref Thread behavior flags.
       * @param stackSize Kernel stack size in bytes; pass `0` for default.
       * @param addressSpace Address space for user stack (`nullptr` for
       *                     kernel-only threads).
       * @return Pointer to the new @ref Thread, or `nullptr` on failure.
       */
      Thread* Create(
        const char* name,
        KernelThreadEntryPoint entryPoint,
        void* argument,
        ThreadPriority priority,
        ThreadFlags flags,
        Size stackSize,
        IAddressSpace* addressSpace = nullptr
      );

      /**
       * @brief Creates a new user-mode @ref Thread.
       * @param name Human-readable @ref Thread name.
       * @param userEntryPoint Virtual address of the user-space entry point.
       * @param userStackTop Top of the pre-allocated user-space stack.
       * @param priority @ref Thread scheduling priority (`0`-`31`).
       * @param addressSpace Address space the thread runs in.
       * @return Pointer to the new @ref Thread, or `nullptr` on failure.
       */
      Thread* CreateUserModeThread(
        const char* name,
        UIntPtr userEntryPoint,
        UIntPtr userStackTop,
        ThreadPriority priority,
        IAddressSpace* addressSpace
      );

      /**
       * @brief Starts a created @ref Thread, adding it to the ready queue.
       * @param thread The @ref Thread to start.
       * @return `true` on success; `false` on failure.
       */
      bool Start(Thread* thread);

      /**
       * @brief Terminates all @ref Thread instances belonging to a process.
       * @param process The process whose @ref Thread instances should be
       *                terminated.
       * @param exitCode The exit code for each terminated @ref Thread.
       */
      void TerminateByProcess(Process* process, Int32 exitCode);

      /**
       * @brief Terminates a @ref Thread.
       * @param thread The @ref Thread to terminate.
       * @param exitCode The exit code for the @ref Thread.
       */
      void Terminate(Thread* thread, Int32 exitCode);

      /**
       * @brief Terminates the current @ref Thread and context-switches to the
       *        next ready @ref Thread.
       * @param context The current interrupt context.
       * @param exitCode The exit code for the current @ref Thread.
       * @return The interrupt context of the next @ref Thread to resume.
       */
      IInterruptContext* Terminate(
        IInterruptContext* context,
        Int32 exitCode
      );

      /**
       * @brief Suspends a @ref Thread.
       * @param thread The @ref Thread to suspend.
       */
      void Suspend(Thread* thread);

      /**
       * @brief Resumes a previously suspended or sleeping @ref Thread.
       * @param thread The @ref Thread to resume.
       */
      void Resume(Thread* thread);

      /**
       * @brief Puts a @ref Thread to sleep for a specified number of timer
       *        ticks.
       * @param thread The @ref Thread to put to sleep.
       * @param ticks Number of ticks to sleep (0 = indefinite).
       */
      void Sleep(Thread* thread, UInt64 ticks);

      /**
       * @brief Yields the current @ref Thread's remaining time slice.
       */
      void Yield();

      /**
       * @brief
       *   Yields the current @ref Thread and immediately context-switches.
       * @param context
       *   Pointer to the current @ref Thread @ref IInterruptContext.
       * @return
       *   Pointer to the @ref IInterruptContext of the next @ref Thread to
       *   resume.
       */
      IInterruptContext* Yield(IInterruptContext* context);

      /**
       * @brief Gets the currently executing @ref Thread.
       * @return Pointer to the current @ref Thread.
       */
      Thread* GetCurrent();

      /**
       * @brief Finds a @ref Thread by its @ref ThreadID.
       * @param id The @ref ThreadID.
       * @return Pointer to the @ref Thread, or `nullptr` if not found.
       */
      Thread* GetByID(ThreadID id);

      /**
       * @brief
       *   Delegates scheduling to the @ref Scheduler and performs context
       *   switches as needed.
       * @param context
       *   Pointer to the current @ref Thread @ref IInterruptContext.
       * @return
       *   Pointer to the @ref IInterruptContext of the @ref Thread that should
       *   resume.
       */
      IInterruptContext* Tick(IInterruptContext* context);

      /**
       * @brief Gets a pointer to the idle @ref Thread.
       * @return Pointer to the idle @ref Thread.
       * @see @ref IdleThread
       */
      Thread* GetIdleThread() const {
        return _idleThread;
      }

      /**
       * @brief Sets the current @ref Thread (for bootstrap only).
       * @param thread The @ref Thread to set as current.
       */
      void SetCurrent(Thread* thread) {
        _current = thread;
        _running = true;
      }

    private:
      /**
       * @brief Reference to the @ref Scheduler for scheduling decisions.
       */
      HybridCFSBitmapScheduler& _scheduler;

      /**
       * @brief Reference to the @ref StackManager for @ref Thread @ref Stack
       *        management.
       */
      StackManager& _stackManager;

      /**
       * @brief
       *   Reference to a concrete implementation of @ref IThreadContextManager
       *   for context management.
       */
      IThreadContextManager& _contextManager;

      /**
       * @brief
       *   Reference to a concrete implementation of @ref ICPUDriver for
       *   CPU-specific operations.
       */
      ICPUDriver& _cpu;

      /**
       * @brief
       *   Reference to a concrete implementation of @ref IMaydayHandler
       *   for mayday handling.
       */
      IMaydayHandler& _maydayHandler;

      /**
       * @brief Reference to the kernel's @ref IAddressSpace.
       */
      IAddressSpace& _kernelAddressSpace;

      /**
       * @brief
       *   Pointer to a concrete implementation of @ref IUserModeManager for
       *   user mode setup and management (may be null if user mode is
       *   unsupported).
       */
      IUserModeManager* _userMode = nullptr;

      /**
       * @brief Pointer to the current @ref Thread.
       */
      Thread* _current = nullptr;

      /**
       * @brief Pointer to the idle @ref Thread.
       * @see @ref IdleThread
       */
      Thread* _idleThread = nullptr;

      /**
       * @brief Array of all @ref Thread instances, indexed by @ref ThreadID.
       */
      Thread* _threads[MaxThreadCount] = {};

      /**
       * @brief @ref IDAllocator for @ref ThreadID.
       */
      IDAllocator<ThreadID, MaxThreadCount> _threadIDs;

      /**
       * @brief @ref Spinlock protecting @ref ThreadManager state.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Pointer to a list of @ref Thread instances waiting to be
       *        cleaned up.
       */
      Thread* _zombieList = nullptr;

      /**
       * @brief List of zombie @ref Thread instances deferred for cleanup
       *        outside the @ref Tick critical section.
       */
      Thread* _deferredCleanupList = nullptr;

      /**
       * @brief Whether threading has been started.
       */
      bool _running = false;

      /**
       * @brief Allocates a new @ref ThreadID.
       * @return The new @ref ThreadID.
       */
      ThreadID _allocateID();

      /**
       * @brief Frees a @ref ThreadID for reuse.
       * @param id The @ref ThreadID to free.
       */
      void _freeID(ThreadID id);

      /**
       * @brief Cleans up a terminated @ref Thread's resources.
       * @param thread The @ref Thread to clean up.
       */
      void _cleanup(Thread* thread);

      /**
       * @brief Processes deferred zombie cleanup outside the @ref Tick lock.
       */
      void _processDeferredCleanup();

      /**
       * @brief
       *   Performs a context switch from the current @ref Thread to the next
       *   @ref Thread.
       * @param context
       *   Pointer to the current @ref Thread's @ref IInterruptContext.
       * @param next
       *   The next @ref Thread to run.
       * @return
       *   Pointer to the @ref IInterruptContext for the next @ref Thread.
       */
      IInterruptContext* _switch(
        IInterruptContext* context,
        Thread* next
      );
  };
}
