/**
 * @file Threading/QuantumOSProcess.cpp
 * @brief Implements @ref @QThrd::OS::IOSProcess.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "OSProcess.hpp"
#include "OSThreadingTypes.hpp"

namespace Quantum::Threading::OS::QuantumOS {
  ProcessID OSProcess::GetCurrentProcessID() {
    KernelClient kernel;

    return static_cast<ProcessID>(kernel.GetProcessID());
  }

  bool OSProcess::GetWorkingDirectory(
    ProcessID pid,
    char* outPath,
    Size outPathSize
  ) {
    RunClient runClient;

    return runClient.GetWorkingDirectory(pid, outPath, outPathSize);
  }

  bool OSProcess::GetProgramDirectory(
    ProcessID pid,
    char* outPath,
    Size outPathSize
  ) {
    RunClient runClient;

    return runClient.GetProgramDirectory(pid, outPath, outPathSize);
  }

  void OSProcess::SetWorkingDirectory(
    ProcessID pid,
    const char* path
  ) {
    RunClient runClient;

    runClient.SetWorkingDirectory(pid, path);
  }
}
