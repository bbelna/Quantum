/**
 * @file Threading/Process.cpp
 * @brief Implements @ref @QThrd::Process.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Process.hpp"

#if defined(OS_QUANTUMOS)
#include "OS/QuantumOS/OSProcess.hpp"

using OSProcess = Quantum::Threading::OS::QuantumOS::OSProcess;
#else
#error "No OS defined"
#endif

namespace Quantum::Threading {
  ProcessID Process::GetCurrentProcessID() {
    return OSProcess::GetCurrentProcessID();
  }

  bool Process::GetWorkingDirectory(
    ProcessID pid,
    char* outPath,
    Size outPathSize
  ) {
    return OSProcess::GetWorkingDirectory(pid, outPath, outPathSize);
  }

  bool Process::GetProgramDirectory(
    ProcessID pid,
    char* outPath,
    Size outPathSize
  ) {
    return OSProcess::GetProgramDirectory(pid, outPath, outPathSize);
  }

  void Process::SetWorkingDirectory(
    ProcessID pid,
    const char* path
  ) {
    OSProcess::SetWorkingDirectory(pid, path);
  }
}
