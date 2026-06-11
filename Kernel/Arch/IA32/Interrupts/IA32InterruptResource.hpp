/**
 * @file Kernel/Arch/IA32/Interrupts/IA32InterruptResource.hpp
 * @brief Declares @ref @QKrnlIA32::Interrupts::IA32InterruptResource.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

#include "Resources/KernelResource.hpp"

namespace Quantum::Kernel::Arch::IA32::Interrupts {
  /**
   * @brief Resource representation for a claimed IA-32 interrupt.
   *
   * Contains the IRQ number that was claimed, as well as per-IRQ state for
   * blocking and waking @ref Thread.
   *
   * Since interrupt claims are exclusive (one @ref Process per IRQ), there
   * is a 1:1 mapping between an @ref IA32InterruptResource and an IRQ line.
   * The @ref IA32InterruptResource itself holds the per-IRQ state (waiting
   * @ref Thread and pending count).
   */
  struct IA32InterruptResource : public KernelResource<UInt8, UInt32> {
    /**
     * @brief Pointer to the @ref Thread currently blocked in an
     *        `Interrupt_Wait` syscall on this IRQ.
     *
     * At most one @ref Thread may wait at a time. Set to `nullptr` when no
     * @ref Thread is waiting.
     */
    Thread* WaitingThread = nullptr;

    /**
     * @brief Number of interrupts that fired while no @ref Thread was
     *        waiting.
     *
     * `Interrupt_Wait` consumes one pending count and returns immediately
     * when this is non-zero.
     */
    UInt32 PendingCount = 0;
  };
}
