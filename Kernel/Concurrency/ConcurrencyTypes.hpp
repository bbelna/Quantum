/**
 * @file Kernel/Concurrency/ConcurrencyTypes.hpp
 * @brief Declares @QKrnl::Concurrency types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Represents an ID for a @ref Thread.
   */
  using ThreadID = UInt32;

  /**
   * @brief Represents an ID for a @ref Process.
   */
  using ProcessID = UInt32;
}
