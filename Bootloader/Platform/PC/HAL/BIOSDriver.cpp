/**
 * @file Bootloader/Platform/PC/HAL/BIOSDriver.cpp
 * @brief Implements @ref @QBtldr::Platform::PC::HAL::BIOSDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "BIOSDriver.hpp"

namespace Quantum::Bootloader::Platform::PC::HAL {
  BIOSRegisters BIOSDriver::CallInterrupt(
    UInt8 vector,
    BIOSRegisters regs
  ) {
    CallBIOS(vector, &regs);

    return regs;
  }
}
