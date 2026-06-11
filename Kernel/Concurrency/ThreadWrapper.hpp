/**
 * @file Kernel/Concurrency/ThreadWrapper.hpp
 * @brief Declares and implements @ref @QKrnl::Concurrency::ThreadWrapper.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "KernelLog.hpp"
#include "ThreadManager.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief @ref Thread wrapper that calls the @ref Thread entry point and
   *        handles exit.
   */
  inline void ThreadWrapper() {
    KernelContext *context = Context;
    Thread* current = context->ThreadManager->GetCurrent();

    if (
      !current ||
      !current->EntryPoint
    ) {
      MAYDAY("ThreadWrapper provided null thread and/or entry point");
    } else {
      // call the actual entry point
      Int32 exitCode = current->EntryPoint(current->EntryArgument);

      // thread has returned, terminate it
      if (context->ThreadManager) {
        context->ThreadManager->Terminate(
          current,
          exitCode
        );
      }

      // should never reach here
      MAYDAY("ThreadWrapper returned");
    }
  }
}
