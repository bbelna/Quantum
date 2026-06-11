/**
 * @file Kernel/Concurrency/HybridCFSBitmapScheduler.hpp
 * @brief Declares @ref @QKrnl::Concurrency::HybridCFSBitmapScheduler.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/SchedulerStats.hpp>

#include "ThreadRunQueue.hpp"
#include "Spinlock.hpp"
#include "Thread.hpp"
#include "ThreadPriority.hpp"
#include "ThreadState.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Hybrid CFS and bitmap priority scheduler.
   *
   * Two-band design:
   *   - **Real-time band (priorities `16`-`31`):** Strict priority via bitmap
   *     with \f$\mathcal{O}(1)\f$ BSR lookup. FIFO within each level. RT
   *     threads always preempt time-sharing threads.
   *   - **Time-sharing band (priorities `0`-`15`):** CFS (Completely Fair
   *     Scheduler) via intrusive red-black tree keyed on virtual runtime.
   *     Higher-priority threads accumulate vruntime slower (proportional
   *     to weight), getting more CPU share. Fairness is a mathematical
   *     invariant, not a heuristic.
   *
   * Also owns the sleep queue and all scheduling decision logic (tick
   * handling, quantum management, yield, sleep, suspend, resume).
   * ThreadManager delegates scheduling decisions here and handles only
   * thread lifecycle and context switching.
   */
  class HybridCFSBitmapScheduler {
    public:
      /**
       * @brief Number of priority levels.
       */
      static constexpr Size PriorityLevelCount = ThreadPriorityCount;

      /**
       * @brief First priority level in the real-time band.
       */
      static constexpr UInt8 RealTimeMinLevel = 16;

      /**
       * @brief Scale factor for virtual runtime accumulation.
       * @note Uses a power of two for efficient division.
       */
      static constexpr UInt64 VRuntimeScale = 1024;

      /**
       * @brief Weight table for CFS time-sharing priorities.
       * @note Higher weight corresponds to slower virtual runtime growth; in
       *       effect, more CPU share.
       * @note Roughly exponential spacing (~1.37x per level).
       */
      static constexpr UInt16 CFSWeightTable[16] = {
        1, // Idle
        8, // UserLowest
        16,
        25,
        36,
        48, // UserLow
        64,
        88,
        121,
        165,
        225, // UserNormal
        307,
        420,
        573,
        783,
        1024 // UserHigh / TimeSharingMax
      };

      /**
       * @brief Target scheduling latency in ticks.
       * @note All scheduled @ref Thread instances should get at least one turn
       *       within this many ticks.
       */
      static constexpr UInt64 CFSTargetLatency = 48;

      /**
       * @brief Minimum quantum a CFS thread can receive, to avoid
       *        context-switch thrashing when many threads are runnable.
       */
      static constexpr UInt8 CFSMinGranularity = 2;

      /**
       * @brief Wake-up vruntime bonus (in scaled units). Waking threads get
       *        their vruntime clamped to `minVruntime - CFSWakeupBonus` so
       *        they run promptly without monopolizing after a long sleep.
       */
      static constexpr UInt64 CFSWakeupBonus = VRuntimeScale * 4;

      /**
       * @brief Quantum table for real-time priorities 16-31.
       *        Indexed by `BasePriority - RealTimeMinLevel`.
       */
      static constexpr UInt8 RTQuantumTable[16] = {
        6, 6, 5, 5, 5, 4, 4, 4,   // 16-23
        4, 4, 3, 3, 3, 2, 1, 1    // 24-31
      };

      /**
       * @brief Rebases CFS vruntimes when the minimum exceeds this
       *        threshold. Prevents vruntime overflow after extended uptime.
       */
      static constexpr UInt64 VirtualRuntimeRebaseThreshold
        = static_cast<UInt64>(1)
       << 62;

      /**
       * @brief Creates a new @ref Scheduler.
       */
      explicit HybridCFSBitmapScheduler();

      /**
       * @brief Destroys the @ref Scheduler.
       */
      ~HybridCFSBitmapScheduler() = default;

      /**
       * @brief Adds a thread to the appropriate ready queue.
       * @param thread The thread to enqueue.
       *
       * Routes to the CFS red-black tree for time-sharing threads or the
       * bitmap FIFO queue for real-time threads.
       */
      void Enqueue(Thread* thread);

      /**
       * @brief Adds a thread to the front of its ready queue.
       * @param thread The thread to enqueue at the front.
       *
       * For RT threads, inserts at the head of the priority level's FIFO.
       * For CFS threads, equivalent to @ref Enqueue (vruntime determines
       * position; wake-up clamping provides the responsiveness boost).
       */
      void EnqueueFront(Thread* thread);

      /**
       * @brief Removes and returns the highest-priority ready thread.
       * @return The next thread to run, or `nullptr` if all queues are empty.
       *
       * RT threads (if any are ready) always take precedence over CFS.
       * Within RT, the highest priority level wins. Within CFS, the thread
       * with the lowest virtual runtime wins.
       */
      Thread* Dequeue();

      /**
       * @brief Removes a specific thread from the ready queue.
       * @param thread The thread to remove.
       * @return `true` if removed; `false` if the thread was not queued.
       */
      bool Remove(Thread* thread);

      /**
       * @brief Checks if all ready queues are empty.
       * @return `true` if empty.
       */
      bool IsEmpty() const;

      /**
       * @brief Gets the total number of threads in all ready queues.
       * @return The thread count.
       */
      Size Count() const;

      /**
       * @brief Charges virtual runtime to a thread.
       * @param thread The thread to charge.
       * @param ticks Number of ticks consumed.
       *
       * Adds `(ticks * VRuntimeScale) / weight` to the thread's vruntime.
       * Only meaningful for time-sharing threads.
       */
      void ChargeVirtualRuntime(Thread* thread, UInt64 ticks);

      /**
       * @brief Clamps a thread's vruntime for wake-up fairness.
       * @param thread The thread to clamp.
       *
       * Sets vruntime to `max(vruntime, minVruntime - CFSWakeupBonus)`.
       * Prevents threads from monopolizing CPU after long sleeps while
       * still giving them a slight head start.
       */
      void ClampVirtualRuntime(Thread* thread);

      /**
       * @brief Initializes vruntime for a newly started thread.
       * @param thread The thread being started.
       *
       * Sets vruntime to the current CFS minimum so new threads start at
       * the fairness floor rather than zero (which would let them
       * monopolize CPU).
       */
      void InitializeVirtualRuntime(Thread* thread);

      /**
       * @brief Computes the quantum for a thread based on its priority band.
       * @param thread The thread to compute quantum for.
       * @return The quantum in ticks.
       *
       * For CFS threads: `max(CFSTargetLatency / numCFSThreads,
       * CFSMinGranularity)`. For RT threads: from @ref RTQuantumTable.
       */
      UInt8 ComputeQuantum(Thread* thread) const;

      /**
       * @brief Gets the current CFS minimum vruntime floor.
       * @return The minimum vruntime.
       */
      UInt64 GetMinVirtualRuntime() const;

      /**
       * @brief Puts a thread to sleep, removing it from the ready queue.
       * @param thread The thread to sleep.
       * @param ticks Number of ticks to sleep. Pass 0 for indefinite sleep
       *              (only woken by @ref Resume).
       */
      void Sleep(Thread* thread, UInt64 ticks);

      /**
       * @brief Suspends a thread, removing it from the ready queue.
       * @param thread The thread to suspend.
       */
      void Suspend(Thread* thread);

      /**
       * @brief Resumes a suspended or sleeping thread.
       * @param thread The thread to resume.
       *
       * For CFS threads, applies vruntime clamping before enqueuing.
       * RT threads get front-enqueued for IPC responsiveness.
       */
      void Resume(Thread* thread);

      /**
       * @brief Gets the number of timed-sleeping threads.
       * @return The sleeping thread count.
       */
      Size SleepingCount() const;

      /**
       * @brief Timer tick handler. Wakes expired sleepers, charges vruntime,
       *        checks quantum expiry, and decides the next thread to run.
       * @param current The currently running thread (may be `nullptr` during
       *                early boot).
       * @return The thread that should run next. May be @p current (no
       *         switch needed), a different thread (switch required), or
       *         `nullptr` (run idle thread).
       *
       * Called from @ref ThreadManager::Tick on every timer interrupt. The
       * caller is responsible for context saving/restoring and the actual
       * context switch.
       */
      Thread* Tick(Thread* current);

      /**
       * @brief Voluntary yield decision. Charges partial vruntime and picks
       *        the next thread.
       * @param current The currently running thread.
       * @return The thread that should run next.
       */
      Thread* Yield(Thread* current);

      /**
       * @brief Fills a @ref SchedulerStats snapshot with current counters.
       * @param stats Pointer to the structure to fill.
       */
      void GetStats(SchedulerStats* stats) const;

      /**
       * @brief Records that the idle thread consumed one tick.
       *        Called by ThreadManager when the idle thread was running.
       */
      void RecordIdleTick();

      /**
       * @brief Records that a context switch occurred.
       *        Called by ThreadManager on every actual switch.
       */
      void RecordContextSwitch();

    private:
      /**
       * @brief Head pointers for each RT priority level's FIFO queue.
       */
      Thread* _heads[PriorityLevelCount] = {};

      /**
       * @brief Tail pointers for each RT priority level's FIFO queue.
       */
      Thread* _tails[PriorityLevelCount] = {};

      /**
       * @brief Bitmap indicating which RT levels have ready threads.
       */
      UInt32 _bitmap = 0;

      /**
       * @brief CFS intrusive red-black tree run queue.
       */
      ThreadRunQueue _cfsQueue;

      /**
       * @brief Total number of threads in all ready queues (RT + CFS).
       */
      Size _totalCount = 0;

      /**
       * @brief Head of the sorted sleep queue (earliest wake time first).
       */
      Thread* _sleepQueueHead = nullptr;

      /**
       * @brief Tail of the sorted sleep queue.
       */
      Thread* _sleepQueueTail = nullptr;

      /**
       * @brief Number of timed-sleeping threads.
       */
      Size _sleepingCount = 0;

      /**
       * @brief Monotonic tick counter incremented on every timer tick.
       */
      UInt64 _tickCount = 0;

      /**
       * @brief Flag indicating a higher-priority thread became ready during
       *        the current tick, requiring preemption.
       */
      bool _higherPriorityReady = false;

      /**
       * @brief Spinlock protecting all scheduler state.
       */
      Spinlock<UInt32> _lock;

      /**
       * @brief Monotonic counter of context switches.
       */
      UInt64 _contextSwitches = 0;

      /**
       * @brief Monotonic counter of idle thread ticks.
       */
      UInt64 _idleTicks = 0;

      /**
       * @brief Monotonic counter of real-time band ticks.
       */
      UInt64 _rtTicks = 0;

      /**
       * @brief Monotonic counter of CFS (time-sharing) band ticks.
       */
      UInt64 _cfsTicks = 0;

      /**
       * @brief Monotonic counter of timed-sleep wakeups.
       */
      UInt64 _sleepWakeups = 0;

      /**
       * @brief Monotonic counter of quantum-expired preemptions.
       */
      UInt64 _preemptions = 0;

      /**
       * @brief Monotonic counter of voluntary yields.
       */
      UInt64 _voluntaryYields = 0;

      /**
       * @brief Enqueues a thread into the RT bitmap queue at the back.
       * @param thread The RT thread to enqueue.
       */
      void _rtEnqueue(Thread* thread);

      /**
       * @brief Enqueues a thread at the front of its RT bitmap queue.
       * @param thread The RT thread to enqueue.
       */
      void _rtEnqueueFront(Thread* thread);

      /**
       * @brief Dequeues the highest-priority RT thread.
       * @return The thread, or `nullptr` if no RT threads are ready.
       */
      Thread* _rtDequeue();

      /**
       * @brief Removes an RT thread from its bitmap queue.
       * @param thread The RT thread to remove.
       * @return `true` if removed.
       */
      bool _rtRemove(Thread* thread);

      /**
       * @brief Checks if any RT threads are ready.
       * @return `true` if the RT bitmap is non-zero.
       */
      bool _rtHasReady() const;

      /**
       * @brief Inserts a thread into the sorted sleep queue by wake time.
       * @param thread The thread to insert (must have SleepWakeTime set).
       */
      void _insertSleepQueue(Thread* thread);

      /**
       * @brief Removes a thread from the sleep queue.
       * @param thread The thread to remove.
       */
      void _removeSleepQueue(Thread* thread);

      /**
       * @brief Wakes all sleepers whose deadline has passed.
       * @param current The currently running thread (for preemption check).
       */
      void _wakeSleepers(Thread* current);

      /**
       * @brief Checks if a thread is in the time-sharing band.
       * @param thread The thread to check.
       * @return `true` if `BasePriority < RealTimeMinLevel`.
       */
      static bool _isCFS(const Thread* thread);
  };
}
