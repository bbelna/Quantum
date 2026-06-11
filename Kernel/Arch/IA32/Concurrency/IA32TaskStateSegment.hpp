/**
 * @file Kernel/Arch/IA32/Concurrency/IA32TaskStateSegment.hpp
 * @brief Declares @ref @QKrnlIA32::Concurrency::IA32TaskStateSegment.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief IA-32 Task State Segment (TSS) structure.
   *
   * The TSS is a hardware-defined 104-byte structure used by the CPU
   * for privilege-level stack switching. On an interrupt or system call
   * that transitions from ring 3 to ring 0, the CPU loads ESP and SS
   * from the TSS's ESP0/SS0 fields. The Quantum kernel uses a single
   * TSS and updates ESP0 on every context switch to point to the
   * current thread's kernel stack top.
   */
  struct [[gnu::packed]] IA32TaskStateSegment {
    /**
     * @brief Previous task link.
     */
    UInt16 PreviousTaskLink;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved0;

    /**
     * @brief Ring 0 stack pointer.
     */
    UInt32 ESP0;

    /**
     * @brief Ring 0 stack segment selector.
     */
    UInt16 SS0;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved1;

    /**
     * @brief Ring 1 stack pointer (unused).
     */
    UInt32 ESP1;

    /**
     * @brief Ring 1 stack segment selector (unused).
     */
    UInt16 SS1;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved2;

    /**
     * @brief Ring 2 stack pointer (unused).
     */
    UInt32 ESP2;

    /**
     * @brief Ring 2 stack segment selector (unused).
     */
    UInt16 SS2;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved3;

    /**
     * @brief Page directory base register (CR3).
     */
    UInt32 CR3;

    /**
     * @brief Instruction pointer.
     */
    UInt32 EIP;

    /**
     * @brief Flags register.
     */
    UInt32 EFLAGS;

    /**
     * @brief General-purpose register EAX.
     */
    UInt32 EAX;

    /**
     * @brief General-purpose register ECX.
     */
    UInt32 ECX;

    /**
     * @brief General-purpose register EDX.
     */
    UInt32 EDX;

    /**
     * @brief General-purpose register EBX.
     */
    UInt32 EBX;

    /**
     * @brief Stack pointer ESP.
     */
    UInt32 ESP;

    /**
     * @brief Base pointer EBP.
     */
    UInt32 EBP;

    /**
     * @brief Source index ESI.
     */
    UInt32 ESI;

    /**
     * @brief Destination index EDI.
     */
    UInt32 EDI;

    /**
     * @brief Extra segment selector ES.
     */
    UInt16 ES;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved4;

    /**
     * @brief Code segment selector CS.
     */
    UInt16 CS;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved5;

    /**
     * @brief Stack segment selector SS.
     */
    UInt16 SS;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved6;

    /**
     * @brief Data segment selector DS.
     */
    UInt16 DS;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved7;

    /**
     * @brief FS segment selector.
     */
    UInt16 FS;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved8;

    /**
     * @brief GS segment selector.
     */
    UInt16 GS;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved9;

    /**
     * @brief LDT segment selector (unused).
     */
    UInt16 LDTSegmentSelector;

    /**
     * @brief Reserved (must be zero).
     */
    UInt16 Reserved10;

    /**
     * @brief Debug trap flag.
     */
    UInt16 DebugTrap;

    /**
     * @brief I/O permission bitmap base offset.
     * 
     * Offset from TSS base to the I/O permission bitmap.
     * Set to `sizeof(TSS)` if no bitmap is used.
     */
    UInt16 IOMapBase;
  };

  static_assert(sizeof(IA32TaskStateSegment) == 104, "TSS must be 104 bytes");
}
