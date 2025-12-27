/**
 * @file System/Drivers/Storage/Floppy/Main.cpp
 * @brief Floppy driver main entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Driver.hpp"

using namespace Quantum::System::Drivers::Storage::Floppy;

int Main() {
  Driver::Main();

  return 0;
}
