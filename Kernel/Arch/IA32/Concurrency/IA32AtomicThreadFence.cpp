/**
 * @file Kernel/Arch/IA32/Concurrency/IA32AtomicThreadFence.cpp
 * @brief Implements
 *        @ref @QKrnlIA32::Concurrency::IA32AtomicThreadFence.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "IA32AtomicThreadFence.hpp"
#include "IA32Atomic.hpp"

namespace Quantum::Kernel::Arch::IA32::Concurrency {
  void IA32AtomicThreadFence(AtomicMemoryOrder order) {
    switch (order) {
      case AtomicMemoryOrder::Relaxed: {
        CompilerBarrier();

        return;
      }

      case AtomicMemoryOrder::Acquire:
      case AtomicMemoryOrder::Release:
      case AtomicMemoryOrder::AcquireRelease: {
        CompilerBarrier();

        return;
      }

      case AtomicMemoryOrder::SequentiallyConsistent: {
        LockedBarrier();

        return;
      }
    }
  }
}
