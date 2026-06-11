/**
 * @file Kernel/Arch/Atomic.hpp
 * @brief Declares @ref @QKrnl::Arch::AtomicT.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#if defined(ARCH_IA32)
#include <Arch/IA32/Concurrency/IA32Atomic.hpp>
#else
#error "Unsupported architecture"
#endif

namespace Quantum::Kernel::Arch {
#if defined(ARCH_IA32)
  /**
   * @brief Architecture-agnostic alias for the per-arch atomic template.
   */
  template <typename T>
  using AtomicT = IA32::Concurrency::IA32Atomic<T>;
  #else
  #error "Unsupported architecture"
  #endif
}
