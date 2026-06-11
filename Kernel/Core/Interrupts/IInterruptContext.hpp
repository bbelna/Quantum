/**
 * @file Kernel/Interrupts/IInterruptContext.hpp
 * @brief Declares @ref @QKrnl::Interrupts::IInterruptContext.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Kernel::Interrupts {
  /**
   * @brief Opaque base structure representing the CPU state at the time of an
   *        interrupt.
   *
   * Architecture-specific implementations (e.g., @ref IA32::InterruptContext)
   * derive from this type and store the full register set, error code, and
   * privilege level. All kernel interrupt handling APIs traffic in
   * @ref IInterruptContext pointers so that platform-independent code can
   * pass contexts through without knowing the concrete layout.
   */
  struct IInterruptContext {};
}
