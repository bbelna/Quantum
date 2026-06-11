/**
 * @file Kernel/Concurrency/Thread.hpp
 * @brief Declares @ref @QKrnl::Concurrency::Thread.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelConstants.hpp>
#include <KernelTypes.hpp>

#include "ThreadFlags.hpp"
#include "ThreadPriority.hpp"
#include "ThreadState.hpp"
#include "ThreadWaitNode.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Function signature for a @ref Kernel @ref Thread entry point.
   * @param argument User-provided argument pointer.
   * @return Exit code.
   */
  using KernelThreadEntryPoint = Int32 (*)(void* argument);

  /**
   * @brief Thread control block containing all per-thread state.
   *
   * Allocated from the kernel heap by @ref ThreadManager::Create. Linked
   * into scheduler ready queues via @ref SchedulerNext / @ref SchedulerPrevious
   * (intrusive doubly-linked list) and into futex wait queues via
   * @ref FutexNext (intrusive singly-linked list).
   */
  struct Thread {
    /**
     * @brief @ref ThreadID for the @ref Thread.
     */
    ThreadID ID = 0;

    /**
     * @brief Name for the @ref Thread.
     */
    char Name[THREAD_NAME_MAX_LENGTH] = {};

    /**
     * @brief The current @ref Thread @ref ThreadState.
     */
    ThreadState State = ThreadState::Created;

    /**
     * @brief Base @ref ThreadPriority for scheduling.
     * @see @ref Scheduler
     *
     * Let \f$p\f$ represent the priority for a @ref Thread. Real-time threads
     * have  \f$p \in [16, 32)\f$ and time-sharing threads have
     * \f$p \in [0, 16)\f$.
     */
    ThreadPriority BasePriority = ThreadPriority::Idle;

    /**
     * @brief CFS virtual runtime (scaled CPU ticks consumed).
     *
     * For time-sharing threads (\f$p \in [0, 16)\f$), this determines
     * scheduling order in the CFS red-black tree. The thread with the
     * lowest virtual runtime runs next. Higher-priority threads accumulate
     * vruntime slower (proportional to weight), giving them more CPU share.
     * Not used for real-time threads (\f$p \in [16, 32)\f$).
     */
    UInt64 VirtualRuntime = 0;

    /**
     * @brief Left @ref Thread child pointer for CFS intrusive red-black tree.
     */
    Thread* RBLeft = nullptr;

    /**
     * @brief Right @ref Thread child pointer for CFS intrusive red-black tree.
     */
    Thread* RBRight = nullptr;

    /**
     * @brief Parent @ref Thread pointer for CFS intrusive red-black tree.
     */
    Thread* RBParent = nullptr;

    /**
     * @brief Red-black tree node color for CFS. `0` = red, `1` = black.
     */
    UInt8 RBNodeColor = 1;

    /**
     * @brief @ref Thread behavior flags.
     */
    ThreadFlags Flags = {};

    /**
     * @brief Pointer to the owning @ref Process, or `nullptr` for kernel
     *        @ref Thread instances.
     */
    Process* OwnerProcess = nullptr;

    /**
     * @brief Pointer to the @ref Thread @ref IInterruptContext.
     */
    IInterruptContext* Context = nullptr;

    /**
     * @brief Pointer to the @ref Thread kernel @ref Stack.
     */
    Stack* KernelStack = nullptr;

    /**
     * @brief Pointer to the @ref Thread user @ref Stack.
     */
    Stack* UserStack = nullptr;

    /**
     * @brief The @ref Thread @ref ThreadEntryPoint.
     */
    KernelThreadEntryPoint EntryPoint = nullptr;

    /**
     * @brief Argument passed to the entry point.
     */
    void* EntryArgument = nullptr;

    /**
     * @brief @ref Thread exit code.
     * @note Valid after termination.
     */
    Int32 ExitCode = 0;

    /**
     * @brief Ticks remaining for sleep, if sleeping.
     */
    UInt64 SleepTicksRemaining = 0;

    /**
     * @brief Absolute tick at which a timed sleep expires. Zero indicates
     *        an indefinite sleep (only woken by Resume).
     */
    UInt64 SleepWakeTime = 0;

    /**
     * @brief Ticks remaining in the current scheduling quantum.
     *        When this reaches zero, the thread is preempted.
     *
     * (OMG, it has Quantum in it!)
     */
    UInt8 QuantumRemaining = 0;

    /**
     * @brief Total CPU ticks consumed by this thread.
     */
    UInt64 TotalCPUTicks = 0;

    /**
     * @brief Pointer to next @ref Thread in @ref Scheduler queue.
     */
    Thread* SchedulerNext = nullptr;

    /**
     * @brief Pointer to previous @ref Thread in @ref Scheduler queue.
     */
    Thread* SchedulerPrevious = nullptr;

    /**
     * @brief Whether this @ref Thread is currently in a @ref Scheduler ready
     *        queue.
     */
    bool InSchedulerQueue = false;

    /**
     * @brief @ref Process address this @ref Thread is waiting on in a futex
     *        wait, or `null` if the @ref Thread is not in any futex wait
     *        queue.
     */
    UIntPtr FutexWaitAddress = 0;

    /**
     * @brief Intrusive singly-linked list @ref Thread pointer for the futex
     *        wait queue.
     */
    Thread* FutexNext = nullptr;

    /**
     * @brief Embedded @ref ThreadWaitNode for IPC send queues.
     */
    ThreadWaitNode SendNode;

    /**
     * @brief Embedded @ref ThreadWaitNode for IPC receive queues (single-port
     *        case).
     */
    ThreadWaitNode ReceiveNode;
  };
}
