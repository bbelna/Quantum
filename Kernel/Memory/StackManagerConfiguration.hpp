/**
 * @file Kernel/Memory/StackManagerConfiguration.hpp
 * @brief Declares @ref @QKrnl::Memory::StackManagerConfiguration.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Configuration parameters for initializing @ref StackManager.
   */
  struct StackManagerConfiguration {
    /**
     * @brief @ref MemoryBlock representing the @ref Stack region.
     */
    MemoryBlock Block;

    /**
     * @brief Number of guard @ref MemoryBlock to place below each @ref Stack.
     */
    Size GuardBlockCount;
  };
}
