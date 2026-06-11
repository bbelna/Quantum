/**
 * @file Bootloader/Platform/PC/HAL/BIOSKeyboardDriver.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::HAL::BIOSKeyboardDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "BIOSKeyboardDriver.hpp"

namespace Quantum::Bootloader::Platform::PC::HAL {
  bool BIOSKeyboardDriver::IsKeyReady() {
    BIOSRegisters regs = {};

    regs.EAX = 0x0100; // AH=01h: check for keystroke

    regs = CallInterrupt(0x16, regs);

    // ZF=1 means buffer empty; ZF=0 means key available
    return (regs.EFLAGS & (1u << 6)) == 0;
  }

  UInt8 BIOSKeyboardDriver::ReadKey() {
    BIOSRegisters regs = {};

    regs.EAX = 0x0000; // AH=00h: read keystroke (blocking)

    regs = CallInterrupt(0x16, regs);

    return static_cast<UInt8>(regs.EAX & 0xFF);
  }
}
