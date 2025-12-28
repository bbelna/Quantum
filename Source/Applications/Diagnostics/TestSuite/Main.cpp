/**
 * @file Applications/Diagnostics/TestSuite/Main.cpp
 * @brief TestSuite application entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Testing.hpp"

using namespace Quantum::Applications::Diagnostics::TestSuite;

int Main() {
  Testing::RegisterBuiltins();
  Testing::RunAll();

  return 0;
}
