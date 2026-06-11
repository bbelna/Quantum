/**
 * @file Kernel/Arch/IA32/Concurrency/IA32AtomicThreadFence.hpp
 * @brief Declares
 *        @ref @QKrnlIA32::Concurrency::IA32AtomicThreadFence.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/AtomicMemoryOrder.hpp>
#include <KernelTypes.hpp>

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  /**
   * @brief Inserts a thread fence with the specified memory order.
   * @param order The memory order for the fence.
   */
  void IA32AtomicThreadFence(
    AtomicMemoryOrder order = AtomicMemoryOrder::SequentiallyConsistent
  );
}
