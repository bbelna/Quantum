/**
 * @file Kernel/Concurrency/StartThreading.hpp
 * @brief Declares @ref @QKrnl::Concurrency::StartThreading.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Bootstraps into the first @ref Thread.
   * @param context Pointer to the @ref Thread @ref IInterruptContext.
   */
  extern "C"
  [[noreturn]]
  void StartThreading(IInterruptContext* context);
}
