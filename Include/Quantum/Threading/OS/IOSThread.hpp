/**
 * @file Include/Quantum/Threading/OS/IOSThread.hpp
 * @brief Interface for platform-specific threading backends.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Threading {
  /**
   * @brief Type for thread entry point functions.
   *
   * The function takes a single `UInt32` argument and returns void. The
   * argument can be used to pass data to the thread at creation time. The
   * thread ID returned by `IOSThread::Create` can be used to identify the
   * thread for operations like joining or signaling, but the entry point
   * function itself does not receive the thread ID as an argument. The thread's
   * execution context (e.g., stack, registers) is managed by the threading
   * backend and is not directly accessible to the entry point function.
   * The entry point is expected to run in user mode with the appropriate
   * permissions and should not perform privileged operations directly.
   * Instead, it should use system calls or other IPC mechanisms to interact
   * with the kernel or other threads as needed.
   */
  using ThreadFunction = void (*)(UInt32);
}

/**
 * @brief Interface and implementations for OS-specific threading backends.
 */
namespace Quantum::Threading::OS {
  /**
   * @brief Interface for OS-specific threading backends.
   */
  class IOSThread {
    public:
      /**
       * @brief Yields execution to allow other threads to run.
       */
      static void Yield();

      /**
       * @brief Puts the calling thread to sleep for the specified number of
       *        timer ticks.
       * @param ticks Number of ticks to sleep.
       */
      static void Sleep(UInt32 ticks);

      /**
       * @brief Creates a new thread in the calling process.
       * @param entryPoint Function to execute in the new thread.
       * @param argument Argument passed to the entry point.
       * @return Thread ID on success, 0 on failure.
       */
      static UInt32 Create(ThreadFunction entryPoint, UInt32 argument = 0);

      /**
       * @brief Terminates the calling thread. If this is the last thread
       *        in the process, the process is terminated with the given
       *        exit code.
       * @param exitCode Exit code for the thread.
       */
      [[noreturn]] static void Exit(Int32 exitCode = 0);
  };
}
