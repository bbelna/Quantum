/**
 * @file Include/Quantum/Kernel/Concurrency/SchedulerStats.hpp
 * @brief Declares @ref @QKrnl::Concurrency::SchedulerStats.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Scheduler performance snapshot.
   *
   * All counters are monotonically increasing and reset only at boot.
   */
  struct SchedulerStats {
    /**
     * @brief Total timer ticks since boot.
     */
    UInt64 TotalTicks;

    /**
     * @brief Total context switches performed.
     */
    UInt64 ContextSwitches;

    /**
     * @brief Ticks spent running the idle thread.
     */
    UInt64 IdleTicks;

    /**
     * @brief Ticks spent running real-time band threads.
     */
    UInt64 RTTicks;

    /**
     * @brief Ticks spent running CFS (time-sharing) band threads.
     */
    UInt64 CFSTicks;

    /**
     * @brief Number of timed-sleep wakeups processed.
     */
    UInt64 SleepWakeups;

    /**
     * @brief Number of quantum-expired preemptions.
     */
    UInt64 Preemptions;

    /**
     * @brief Number of voluntary yields.
     */
    UInt64 VoluntaryYields;

    /**
     * @brief Current number of threads in the ready queue (RT + CFS).
     */
    UInt32 ReadyCount;

    /**
     * @brief Current number of timed-sleeping threads.
     */
    UInt32 SleepingCount;
  };
}
