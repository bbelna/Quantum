/**
 * @file Utilities/kmem/Main.cpp
 * @brief Main entry point for the kmem command.
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
using namespace Quantum::Kernel::ABI;
using namespace Quantum::Kernel::Memory::ABI;
using namespace Quantum::Streaming;

/**
 * @brief Main entry point for the kernel memory command (`kmem`).
 *
 * Displays detailed kernel memory diagnostics including physical memory
 * overview, per-tag block allocation breakdown, kernel heap usage,
 * memory pressure state, and IPC/shared buffer pool statistics.
 */
int Main() {
  if (!StandardOut.IsValid()) return 1;

  char line[128];
  KernelClient kernel;
  KernelMemoryInfo info;

  if (!kernel.GetMemoryInfo(&info)) {
    StandardOut.Write("Could not retrieve memory information\n");

    return 1;
  }

  // compute a shared column width for all indented () rows:
  // pressure sub-labels and memory tag names
  constexpr Size maxLabelWidth = 20;
  const char* pressureLabels[] = {
    "Pressure",
    "IPCMessage",
    "IPCPort",
    "SharedBuffers"
  };
  Size labelWidth = 0;

  for (const char* label : pressureLabels) {
    Size length = CString::Length(label);

    if (length > labelWidth) labelWidth = length;
  }

  MemoryTagStats tags;
  bool hasTagStats = GetTagStats(&tags);

  if (hasTagStats) {
    for (Size tagIndex = 0; tagIndex < MaxBlockTags; tagIndex++) {
      Size length = CString::Length(tags.TagNames[tagIndex]);

      if (length > labelWidth) labelWidth = length;
    }
  }

  if (labelWidth > maxLabelWidth) labelWidth = maxLabelWidth;

  char labelColumn[maxLabelWidth + 1];
  MemoryPressureInfo pressure;

  if (GetPressureInfo(&pressure)) {
    const char* stateString = "???";

    if (pressure.State == PressureState::Normal) {
      stateString = "Normal";
    } else if (pressure.State == PressureState::Elevated) {
      stateString = "Elevated";
    } else if (pressure.State == PressureState::Critical) {
      stateString = "Critical";
    }

    CString::PadRight(
      "Pressure",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %s\n",
      labelColumn,
      stateString
    );
    StandardOut.Write(line);

    CString::PadRight(
      "User",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u KB\n",
      labelColumn,
      info.UserProcessBytes / 1024
    );
    StandardOut.Write(line);

    CString::PadRight(
      "Heap",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u KB\n",
      labelColumn,
      info.KernelHeapBytes / 1024
    );
    StandardOut.Write(line);

    CString::PadRight(
      "Image",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u KB\n",
      labelColumn,
      info.InitialImageBytes / 1024
    );
    StandardOut.Write(line);

    CString::PadRight(
      "Reserved",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u KB\n",
      labelColumn,
      info.KernelReservedBytes / 1024
    );
    StandardOut.Write(line);

    CString::PadRight(
      "IPCMessage",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u B\n",
      labelColumn,
      pressure.PoolIPCMessageBytes
    );
    StandardOut.Write(line);

    CString::PadRight(
      "IPCPort",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u B\n",
      labelColumn,
      pressure.PoolIPCPortBytes
    );
    StandardOut.Write(line);

    CString::PadRight(
      "SharedBuffers",
      labelColumn,
      sizeof(labelColumn),
      labelWidth
    );
    CString::Format(
      line,
      sizeof(line),
      "%s  %u B\n",
      labelColumn,
      pressure.PoolSharedBufferBytes
    );
    StandardOut.Write(line);
  }

  if (hasTagStats) {
    for (Size tagIndex = 0; tagIndex < MaxBlockTags; tagIndex++) {
      UInt32 tagKB = (tags.BlockCounts[tagIndex] * tags.BlockSize) / 1024;

      CString::PadRight(
        tags.TagNames[tagIndex],
        labelColumn,
        sizeof(labelColumn),
        labelWidth
      );
      CString::Format(
        line,
        sizeof(line),
        "%s  %u KB (%u blocks)\n",
        labelColumn,
        tagKB,
        tags.BlockCounts[tagIndex]
      );

      StandardOut.Write(line);
    }
  }

  return 0;
}
