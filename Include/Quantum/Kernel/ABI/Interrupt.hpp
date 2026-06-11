/**
 * @file Include/Quantum/Kernel/ABI/Interrupt.hpp
 * @brief Declaration of the kernel interrupt ABI.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Kernel/Types.hpp>
#include <Quantum/Core/Types.hpp>

#include "ABI.hpp"

/**
 * @brief ABI functions for hardware interrupts.
 */
namespace Quantum::Kernel::ABI::Interrupt {
  /**
   * @brief Claims a hardware IRQ for the calling process.
   * @param irq The IRQ number to claim (0-15).
   * @return A resource handle for the claimed interrupt, or -1 on failure.
   *
   * The calling process must have `ProcessPermissions::Interrupts`. Each IRQ
   * can only be claimed by one process at a time.
   */
  inline Kernel::Resources::ResourceID Claim(UInt8 irq) {
    return Invoke(
      KernelOperation::Interrupt_Claim,
      static_cast<UInt32>(irq)
    );
  }

  /**
   * @brief Waits for the next interrupt on a claimed IRQ.
   * @param handle The resource handle returned by `Claim`.
   *
   * Blocks the calling thread until the interrupt fires. If an interrupt
   * arrived while no thread was waiting, returns immediately.
   */
  inline void Wait(Kernel::Resources::ResourceID handle) {
    Invoke(
      KernelOperation::Interrupt_Wait,
      handle
    );
  }

  /**
   * @brief Releases a previously claimed hardware IRQ.
   * @param handle The resource handle returned by `Claim`.
   *
   * Masks the IRQ, removes the kernel handler, and frees the resource.
   */
  inline void Release(Kernel::Resources::ResourceID handle) {
    Invoke(
      KernelOperation::Interrupt_Release,
      handle
    );
  }
}
