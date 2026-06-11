/**
 * @file Bootloader/Platform/PC/HAL/BIOS.hpp
 * @brief Interface for calling real-mode BIOS interrupts from 32-bit
 *        protected mode via the BIOS trampoline.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

/**
 * @brief Low-level BIOS trampoline interface for the PC bootloader.
 *
 * Provides primitives for invoking real-mode BIOS interrupts from 32-bit
 * protected mode. The trampoline is implemented in BIOS.asm;
 * @ref InitializeBIOS must be called once before any @ref CallBIOS use.
 */
namespace Quantum::Bootloader::Platform::PC::HAL::BIOS {
  /**
   * @brief Register state for BIOS interrupt calls.
   * @note Struct layout must match the offsets used in BIOS.asm.
   *
   * On input, set the registers needed by the BIOS function. On output,
   * all fields are updated with the values returned by the BIOS. Check
   * `EFLAGS` bit 0 (`CF`) for error indication.
   */
  struct BIOSRegisters {
    /**
     * @brief General-purpose register A (offset +0).
     */
    UInt32 EAX;

    /**
     * @brief General-purpose register B (offset +4).
     */
    UInt32 EBX;

    /**
     * @brief General-purpose register C (offset +8).
     */
    UInt32 ECX;

    /**
     * @brief General-purpose register D (offset +12).
     */
    UInt32 EDX;

    /**
     * @brief Source index register (offset +16).
     */
    UInt32 ESI;

    /**
     * @brief Destination index register (offset +20).
     */
    UInt32 EDI;

    /**
     * @brief Data segment selector (offset +24).
     */
    UInt16 DS;

    /**
     * @brief Extra segment selector (offset +26).
     */
    UInt16 ES;

    /**
     * @brief CPU flags (offset +28, output only).
     *
     * Check bit 0 (`CF`) after a BIOS call to detect errors.
     */
    UInt32 EFLAGS;
  };

  /**
   * @brief Loads the bootloader's Global Descriptor Table (GDT) with 16-bit
   *        segments for the trampoline.
   * @note Defined in BIOS.asm as a C-linkage symbol.
   *
   * Installs a five-entry GDT containing null, 32-bit code/data, and
   * 16-bit code/data descriptors. Must be called once before any
   * @ref CallBIOS use.
   */
  extern "C" void InitializeBIOS();

  /**
   * @brief Calls a real-mode BIOS interrupt from 32-bit protected mode.
   * @param interruptNumber The BIOS interrupt vector (e.g. `0x10`, `0x13`,
   *                        `0x15`).
   * @param regs Pointer to register state (input/output).
   * @note Defined in BIOS.asm as a C-linkage symbol.
   *
   * Temporarily switches PM32 -> PM16 -> real mode, loads registers from
   * the @p regs struct, executes the interrupt, saves the output registers
   * back, and returns to PM32.
   */
  extern "C" void CallBIOS(UInt8 interruptNumber, BIOSRegisters* regs);

  /**
   * @brief Returns the BIOS tick counter from `INT 1Ah`/`AH=00h`.
   * @return The number of ticks since midnight, where each tick is
   *         \f$\approx 55 \text{ ms}\f$.
   *
   * The value is the 32-bit `CX:DX` tick count since midnight. Note that
   * the counter wraps around every 24 hours (`0x18EE7E00` ticks).
   *
   * The timer is driven by the PIT channel 0, which generates IRQ 0 at a
   * rate of \f$18.2 \text{ Hz}\f$. This function is used to implement a
   * simple timeout mechanism in the boot prompt by polling the tick count
   * and checking for elapsed time.
   */
  inline UInt32 GetBIOSClockTicks() {
    BIOSRegisters regs = {};

    regs.EAX = 0x0000; // AH=00h: get system timer count

    CallBIOS(0x1A, &regs);

    return
      ((regs.ECX & 0xFFFF) << 16)
      | (regs.EDX & 0xFFFF);
  }
}
