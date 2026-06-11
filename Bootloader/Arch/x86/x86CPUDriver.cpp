/**
 * @file Bootloader/Arch/x86/x86CPUDriver.cpp
 * @brief Implements @ref @QBtldr::Arch::x86::x86CPUDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "x86CPUDriver.hpp"

namespace Quantum::Bootloader::Arch::x86 {
  [[noreturn]] void x86CPUDriver::HaltForever() {
    asm volatile(
      "cli\n"
      "1: hlt\n"
      "jmp 1b\n"
    );

    __builtin_unreachable();
  }
}
