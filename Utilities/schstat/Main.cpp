/**
 * @file Utilities/schstat/Main.cpp
 * @brief Main entry point for the schstat command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Core.hpp>
#include <Quantum/Clients.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Streaming.hpp>

using namespace Quantum::Core;
using namespace Quantum::Clients;
using namespace Quantum::Kernel::Concurrency;
using namespace Quantum::Streaming;

int Main() {
  if (!StandardOut.IsValid()) return 1;

  char line[128];
  KernelClient kernel;
  SchedulerStats stats;

  if (!kernel.GetSchedulerStats(&stats)) {
    StandardOut.Write("Could not retrieve scheduler statistics\n");

    return 1;
  }

  constexpr Size labelWidth = 20;
  char labelColumn[labelWidth + 1];

  CString::PadRight(
    "TotalTicks",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.TotalTicks)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "ContextSwitches",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.ContextSwitches)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "IdleTicks",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.IdleTicks)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "RTTicks",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.RTTicks)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "CFSTicks",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.CFSTicks)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "SleepWakeups",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.SleepWakeups)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "Preemptions",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.Preemptions)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "VoluntaryYields",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    static_cast<UInt32>(stats.VoluntaryYields)
  );
  StandardOut.Write(line);

  CString::PadRight(
    "ReadyCount",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    stats.ReadyCount
  );
  StandardOut.Write(line);

  CString::PadRight(
    "SleepingCount",
    labelColumn,
    sizeof(labelColumn),
    labelWidth
  );
  CString::Format(
    line,
    sizeof(line),
    "%s  %u\n",
    labelColumn,
    stats.SleepingCount
  );
  StandardOut.Write(line);

  return 0;
}
