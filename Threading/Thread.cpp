/**
 * @file Threading/Thread.cpp
 * @brief Implements @ref @QThrd::Thread.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Thread.hpp"

#if defined(OS_QUANTUMOS)
#include "OS/QuantumOS/OSThread.hpp"

using OSThread = Quantum::Threading::OS::QuantumOS::OSThread;
#else
#error "No OS defined"
#endif

namespace Quantum::Threading {
  void Thread::Yield() {
    OSThread::Yield();
  }

  void Thread::Sleep(UInt32 ticks) {
    OSThread::Sleep(ticks);
  }

  UInt32 Thread::Create(ThreadFunction entryPoint, UInt32 argument) {
    return OSThread::Create(entryPoint, argument);
  }

  void Thread::Exit(Int32 exitCode) {
    OSThread::Exit(exitCode);
  }
}
