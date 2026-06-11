/**
 * @file Bootloader/Platform/PC/HAL/BIOSTimerDriver.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::HAL::BIOSTimerDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "BIOSTimerDriver.hpp"

namespace Quantum::Bootloader::Platform::PC::HAL {
  UInt32 BIOSTimerDriver::GetMS() {
    BIOSRegisters regs = {};

    regs.EAX = 0x0000; // AH=00h: get system timer count

    regs = CallInterrupt(0x1A, regs);

    UInt32 ticks =
      ((regs.ECX & 0xFFFF) << 16)
      | (regs.EDX & 0xFFFF);

    // ~18.2 ticks/sec -> ticks * 10000 / 182
    return ticks * 10000 / 182;
  }

  void BIOSTimerDriver::WaitMS(UInt32 ms) {
    UInt32 start = GetMS();

    while (GetMS() - start < ms) {}
  }
}
