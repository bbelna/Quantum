/**
 * @file Kernel/Interrupts/InterruptTypes.hpp
 * @brief Declares @ref @QKrnl::Interrupts types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "IInterruptContext.hpp"

namespace Quantum::Kernel::Interrupts {
  /**
   * @brief Represents an interrupt vector number.
   */
  using InterruptVector = UInt8;

  /**
   * @brief Function pointer type for interrupt handler callbacks.
   *
   * An interrupt handler receives a reference to the current
   * @ref IInterruptContext and returns a pointer to the context that
   * should be restored when the handler returns. Returning a different
   * context pointer triggers a context switch (e.g., during scheduling).
   */
  using InterruptHandler = IInterruptContext* (*)(IInterruptContext&);
}
