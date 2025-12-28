/**
 * @file System/Drivers/Input/PS2/Keyboard/Main.cpp
 * @brief PS/2 keyboard driver entry point.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Driver.hpp"

using namespace Quantum::System::Drivers::Input::PS2::Keyboard;

int Main() {
  Driver::Main();

  return 0;
}
