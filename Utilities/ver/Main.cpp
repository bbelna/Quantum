/**
 * @file Utilities/ver/Main.cpp
 * @brief Main entry point for the version command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Version.hpp>
#include <Quantum/Streaming.hpp>

using namespace Quantum::Streaming;

/**
 * @brief Main entry point for the version utility.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Prints the QuantumOS version, platform, architecture, and build
 * timestamp.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  StandardOut.Write(
    "Quantum " QUANTUMOS_RELEASE " " PLATFORM "/" TARGET_ARCH " "
    __DATE__ " " __TIME__ "\n"
  );

  return 0;
}
