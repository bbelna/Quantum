/**
 * @file Include/Quantum/Kernel/Concurrency/ProcessConstants.hpp
 * @brief Declares constants for @ref @QKrnl::Concurrency.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "ConcurrencyTypes.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Invalid @ref ProcessID value.
   */
  inline constexpr ProcessID InvalidProcessID = static_cast<ProcessID>(-1);
}
