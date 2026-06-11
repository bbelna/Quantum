/**
 * @file Include/Quantum/Threading/Thread.hpp
 * @brief Declaration of core threading primitives.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Threading/OS/IOSThread.hpp>

namespace Quantum::Threading {
  /**
   * @brief Represents a thread of execution.
   */
  class Thread {
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