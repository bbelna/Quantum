/**
 * @file System/Kernel/Include/Objects/IRQLineObject.hpp
 * @brief IRQ line kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/KernelObject.hpp"

namespace Quantum::System::Kernel::Objects {
  /**
   * IRQ line kernel object.
   */
  class IRQLineObject : public KernelObject {
    public:
      /**
       * Constructs an IRQ line object.
       * @param irq
       *   IRQ line number.
       */
      explicit IRQLineObject(UInt32 irq);

      /**
       * IRQ line number.
       */
      UInt32 irqLine;
  };
}
