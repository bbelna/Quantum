/**
 * @file Include/Quantum/Kernel/Concurrency/ProcessInfo.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ProcessInfo.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "ConcurrencyTypes.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Information about a running process.
   */
  struct ProcessInfo {
    /**
     * @brief The process ID.
     */
    ProcessID ID;

    /**
     * @brief The process name.
     */
    char Name[64];

    /**
     * @brief The process state (maps to kernel `ProcessState` enum).
     */
    UInt8 State;

    /**
     * @brief Number of active threads in this process.
     */
    Size ThreadCount;

    /**
     * @brief Number of heap pages mapped to this process.
     */
    Size HeapPageCount;

    /**
     * @brief Total number of physical pages mapped to this process.
     */
    Size TotalPageCount;
  };
}
