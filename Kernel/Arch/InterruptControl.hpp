/**
 * @file Kernel/Arch/InterruptControl.hpp
 * @brief Declares @ref @QKrnl::Arch::InterruptControl.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#if defined(ARCH_IA32)
#include <Arch/IA32/Interrupts/IA32InterruptControl.hpp>
#else
#error "Unsupported architecture"
#endif

namespace Quantum::Kernel::Arch {
  #if defined(ARCH_IA32)
  /**
   * @brief Architecture-agnostic alias for the per-arch interrupt-control
   *        type.
   */
  using InterruptControl = IA32::Interrupts::IA32InterruptControl;
  #else
  #error "Unsupported architecture"
  #endif
}
