/**
 * @file Bootloader/Platform/PC/HAL/BIOSDriver.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::HAL::BIOSDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Platform/PC/HAL/PCHALTypes.hpp>

namespace Quantum::Bootloader::Platform::PC::HAL {
  /**
   * @brief Base class for HAL drivers that interact with the BIOS via the
   *        real-mode trampoline.
   *
   * Provides a single protected primitive, @ref CallInterrupt, that
   * subclasses use to invoke BIOS interrupt services without depending
   * directly on @ref CallBIOS or @ref BIOSRegisters at their call
   * sites.
   */
  class BIOSDriver {
    protected:
      /**
       * @brief Calls a BIOS interrupt and returns the resulting register
       *        state.
       * @param vector The interrupt vector to invoke (e.g. `0x10`, `0x16`).
       * @param regs   Input register state; populated by the caller before
       *               the call.
       * @return The register state as written by the BIOS on return,
       *         including updated general-purpose registers and `EFLAGS`.
       *
       * Delegates to @ref CallBIOS via the real-mode trampoline. The
       * @p regs argument is passed by value so callers can construct it
       * inline without keeping a mutable local.
       */
      BIOSRegisters CallInterrupt(UInt8 vector, BIOSRegisters regs);
  };
}
