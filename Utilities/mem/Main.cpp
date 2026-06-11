/**
 * @file Utilities/mem/Main.cpp
 * @brief Main entry point for the memory command.
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
using namespace Quantum::Streaming;

/**
 * @brief Main entry point for the memory command (`mem`).
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Displays a summary of system memory usage. Use `kmem` for detailed
 * kernel memory diagnostics.
 */
int Main() {
  if (!StandardOut.IsValid()) return 1;

  KernelClient kernel;
  KernelMemoryInfo info;

  if (!kernel.GetMemoryInfo(&info)) {
    StandardOut.Write("Error: could not retrieve memory information\n");

    return 1;
  }

  char line[128];

  CString::Format(
    line, sizeof(line),
    "Total  %u KB (%u x %u B)\n",
    info.TotalBytes / 1024,
    info.BlockCount,
    info.BlockSize
  );
  StandardOut.Write(line);

  CString::Format(
    line, sizeof(line),
    "Used   %u KB (%u blocks)\n",
    info.UsedBytes / 1024,
    info.UsedBlockCount
  );
  StandardOut.Write(line);

  CString::Format(
    line, sizeof(line),
    "Free   %u KB (%u blocks)\n",
    info.FreeBytes / 1024,
    info.BlockCount - info.UsedBlockCount
  );
  StandardOut.Write(line);

  return 0;
}
