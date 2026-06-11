/**
 * @file Include/Quantum/Kernel/ABI/Thread.hpp
 * @brief Declaration of the kernel thread ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ABI.hpp"

/**
 * @brief ABI functions for thread management and synchronization.
 */
namespace Quantum::Kernel::ABI::Thread {
  /**
   * @brief Yields the current thread's time slice to the scheduler.
   */
  inline void Yield() {
    Invoke(KernelOperation::Thread_Yield);
  }

  /**
   * @brief Puts the calling thread to sleep for the specified number of
   *        timer ticks.
   * @param ticks Number of ticks to sleep.
   */
  inline void Sleep(UInt32 ticks) {
    Invoke(KernelOperation::Thread_Sleep, ticks);
  }

  /**
   * @brief Creates a new thread in the calling process.
   * @param entryPoint User-space entry point address.
   * @param argument Argument passed to the entry point.
   * @return Thread ID on success, 0 on failure.
   */
  inline UInt32 Create(UInt32 entryPoint, UInt32 argument) {
    return Invoke(KernelOperation::Thread_Create, entryPoint, argument);
  }

  /**
   * @brief Atomically checks `*address == expected` and, if true, suspends
   *        the calling thread until woken by `FutexWake`.
   * @param address Pointer to a UInt32 in user memory.
   * @param expected The value to compare against.
   * @return 0 if woken by FutexWake, 1 if the value did not match.
   */
  inline UInt32 FutexWait(volatile UInt32* address, UInt32 expected) {
    return Invoke(
      KernelOperation::Thread_FutexWait,
      reinterpret_cast<UInt32>(address),
      expected
    );
  }

  /**
   * @brief Wakes up to `count` threads blocked in `FutexWait` on `address`.
   * @param address Pointer to the same UInt32 passed to FutexWait.
   * @param count Maximum number of threads to wake.
   * @return Number of threads actually woken.
   */
  inline UInt32 FutexWake(volatile UInt32* address, UInt32 count) {
    return Invoke(
      KernelOperation::Thread_FutexWake,
      reinterpret_cast<UInt32>(address),
      count
    );
  }

  /**
   * @brief Sets the scheduling priority of the calling thread.
   * @param priority 0-15 = time-sharing, 16-31 = real-time.
   * @return 0 on success, 1 on invalid priority.
   */
  inline UInt32 SetPriority(UInt32 priority) {
    return Invoke(KernelOperation::Thread_SetPriority, priority);
  }

  namespace Priority {
    constexpr UInt32 Idle = 0;
    constexpr UInt32 UserLowest = 1;
    constexpr UInt32 UserLow = 5;
    constexpr UInt32 UserNormal = 10;
    constexpr UInt32 UserHigh = 15;
    constexpr UInt32 ServerBase = 16;
    constexpr UInt32 GraphicsServer = 20;
    constexpr UInt32 InputDriver = 22;
    constexpr UInt32 InputServer = 24;
    constexpr UInt32 DisplayServerRender = 25;
    constexpr UInt32 DisplayServer = 27;
  }
}
