/**
 * @file Utilities/echo/Main.cpp
 * @brief Main entry point for the echo command.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Quantum/Streaming.hpp>

using namespace Quantum::Streaming;

/**
 * @brief Main entry point for the Echo utility.
 * @param argumentCount Number of arguments.
 * @param arguments Argument vector. `[1..N]` are the strings to echo.
 * @return Exit code (`0` on success, `1` on failure).
 *
 * Writes all arguments to stdout, separated by spaces, followed by a
 * newline.
 */
int Main(int argumentCount, char** arguments, char** environment) {
  if (!StandardOut.IsValid()) return 1;

  // echo all arguments (starting at [1]) separated by spaces
  if (argumentCount > 1) {
    for (int index = 1; index < argumentCount; ++index) {
      if (index > 1) {
        StandardOut.Write(" ", 1);
      }

      StandardOut.Write(arguments[index]);
    }

    StandardOut.Write("\n", 1);
  }

  return 0;
}
