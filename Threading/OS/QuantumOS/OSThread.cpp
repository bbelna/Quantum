/**
 * @file Threading/OS/QuantumOS/OSThread.cpp
 * @brief QuantumOS implementation of @ref Quantum::Threading::OS::IOSThread.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "OS/QuantumOS/OSThreadingTypes.hpp"

#include "OSThread.hpp"

namespace Quantum::Threading::OS::QuantumOS {
  void OSThread::Yield() {
    KernelClient kernel;

    kernel.YieldThread();
  }

  void OSThread::Sleep(UInt32 ticks) {
    KernelClient kernel;

    kernel.SleepThread(ticks);
  }

  UInt32 OSThread::Create(ThreadFunction entryPoint, UInt32 argument) {
    KernelClient kernel;

    return kernel.CreateThread(
      reinterpret_cast<UInt32>(entryPoint),
      argument
    );
  }

  void OSThread::Exit(Int32 exitCode) {
    KernelClient kernel;

    kernel.ExitThread(exitCode);
  }
}
