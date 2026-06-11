/**
 * @file Utilities/qsh/Main.cpp
 * @brief Main entry point for the @ref @Q:Shell.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "Shell.hpp"

using namespace Quantum::Shell;

/**
 * @brief Main entry point for the @ref @Q:Shell.
 * @return @ref @Q:Shell exit code.
 */
int Main() {
  Shell shell;

  return shell.Run();
}
