/**
 * @file Include/Quantum/Kernel/Memory/MemoryTagStat.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryTagStat.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Per-tag memory usage snapshot.
   */
  struct MemoryTagStat {
    /**
     * @brief Block size in bytes.
     */
    Size BlockSize;

    /**
     * @brief Number of allocated blocks for each tag index.
     */
    Size BlockCounts[12];

    /**
     * @brief Human-readable label for each tag index.
     */
    char TagNames[12][20];
  };
}
