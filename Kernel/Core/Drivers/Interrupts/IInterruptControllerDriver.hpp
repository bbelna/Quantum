/**
 * @file Kernel/Drivers/Interrupts/IInterruptControllerDriver.hpp
 * @brief Declares @ref @QKrnl::Drivers::Interrupts::IInterruptControllerDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Drivers/DriverTypes.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Drivers::Interrupts {
  /**
   * @brief Abstract device driver interface for an interrupt controller device.
   *
   * Provides operations for acknowledging interrupts (EOI), masking and
   * unmasking individual or all interrupt lines, and querying the base
   * vector offset. Concrete implementations (e.g., @ref Intel8259Driver) program
   * the hardware controller registers.
   */
  template<typename InterruptVectorType>
  class IInterruptControllerDriver : public IDriver {
    public:
      /**
       * @brief Destroys this @ref IInterruptControllerDriver instance.
       */
      virtual ~IInterruptControllerDriver() = default;

      /**
       * @brief Sends an end-of-interrupt (EOI) signal for the given vector.
       * @param vector The interrupt vector to acknowledge.
       *
       * Must be called at the end of every hardware interrupt handler to
       * inform the controller that the interrupt has been serviced and
       * lower-priority interrupts may be delivered again.
       */
      virtual void End(InterruptVectorType vector) = 0;

      /**
       * @brief Masks (disables) the given interrupt vector.
       * @param vector The interrupt vector to mask.
       */
      virtual void Mask(InterruptVectorType vector) = 0;

      /**
       * @brief Masks (disables) all interrupt vectors.
       */
      virtual void MaskAll() = 0;

      /**
       * @brief Unmasks (enables) the given interrupt vector.
       * @param vector The interrupt vector to unmask.
       */
      virtual void Unmask(InterruptVectorType vector) = 0;

      /**
       * @brief Unmasks (enables) all interrupt vectors.
       */
      virtual void UnmaskAll() = 0;

      /**
       * @brief Returns the base interrupt vector for hardware IRQs.
       * @return The base vector number.
       */
      virtual UInt8 GetBaseVector() = 0;
  };
}
