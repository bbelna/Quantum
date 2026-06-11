/**
 * @file Utilities/cwd/Main.cpp
 * @brief Main entry point for the current working directory command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Streaming.hpp>
#include <Quantum/Threading.hpp>

using namespace Quantum::Streaming;
using namespace Quantum::Threading;

/**
 * @brief Main entry point for the current working directory command.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Queries the run server for the calling process's working directory and
 * writes it to stdout, followed by a newline.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  char workingDirectory[256];

  bool success = Process::GetWorkingDirectory(
    Process::GetCurrentProcessID(),
    workingDirectory,
    sizeof(workingDirectory)
  );

  if (success && workingDirectory[0] != '\0') {
    StandardOut.Write(workingDirectory);
  } else {
    StandardOut.Write("/");
  }

  StandardOut.Write("\n", 1);

  return 0;
}
