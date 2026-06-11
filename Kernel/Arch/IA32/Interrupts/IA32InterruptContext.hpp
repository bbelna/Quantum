/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptContext.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::InterruptContext.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include <Interrupts/IInterruptContext.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief Register snapshot captured on interrupt entry for IA-32.
   *
   * Layout matches the stack frame built by the ISR stubs: segment
   * registers pushed first (`GS`, `FS`, `ES`, `DS`), then `pusha` (`EDI`
   * through `EAX`), then the software-pushed vector and error code, and finally
   * the CPU-pushed `EIP`, `CS`, and `EFLAGS`.
   *
   * For ring 3 to ring 0 transitions the CPU also pushes `ESP` and `SS`
   * after `EFLAGS`, but those are not part of this struct because `iret`
   * handles them automatically based on the privilege level change
   * detected from `CS`. User mode context switching is handled separately
   * via `PrepareThread`.
   */
  struct IA32InterruptContext : public IInterruptContext {
    /**
     * @brief Checks if an interrupt context came from user mode.
     * @param context The interrupt context to check.
     * @return `true` if the context is from user mode (ring 3).
     */
    static bool IsUserModeContext(const IA32InterruptContext& context) {
      return (context.CS & 0x3) == 3;
    }

    /**
     * @brief Saved `GS` segment register.
     */
    UInt32 GS;

    /**
     * @brief Saved `FS` segment register.
     */
    UInt32 FS;

    /**
     * @brief Saved `ES` segment register.
     */
    UInt32 ES;

    /**
     * @brief Saved `DS` segment register.
     */
    UInt32 DS;

    /**
     * @brief General-purpose register `EDI` (pusha order).
     */
    UInt32 EDI;

    /**
     * @brief General-purpose register `ESI` (pusha order).
     */
    UInt32 ESI;

    /**
     * @brief Base pointer captured during pusha.
     */
    UInt32 EBP;

    /**
     * @brief Value before pusha for `ESP`.
     */
    UInt32 ESP;

    /**
     * @brief General-purpose register `EBX` (pusha order).
     */
    UInt32 EBX;

    /**
     * @brief General-purpose register `EDX` (pusha order).
     */
    UInt32 EDX;

    /**
     * @brief General-purpose register `ECX` (pusha order).
     */
    UInt32 ECX;

    /**
     * @brief General-purpose register `EAX` (pusha order).
     */
    UInt32 EAX;

    /**
     * @brief Software-pushed vector.
     */
    UInt32 Vector;

    /**
     * @brief Software-pushed hardware/synthetic error code.
     */
    UInt32 ErrorCode;

    /**
     * @brief Instruction pointer at the time of the interrupt.
     */
    UInt32 EIP;

    /**
     * @brief Code segment selector at the time of the interrupt.
     */
    UInt32 CS;

    /**
     * @brief `EFLAGS` register at the time of the interrupt.
     */
    UInt32 EFLAGS;

    // note: for ring 3 -> ring 0 transitions, the CPU also pushes `ESP` and
    // `SS` after `EFLAGS`; these are NOT part of this struct because `iret`
    // handles them automatically based on the privilege level change detected
    // from `CS` -- user mode context switching is handled separately via
    // `PrepareThread`
  };
}
