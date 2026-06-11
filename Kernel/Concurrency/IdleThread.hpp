/**
 * @file Kernel/Concurrency/IdleThread.hpp
 * @brief Declares and implements @ref @QKrnl::Concurrency::IdleThread.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Drivers/CPU/ICPUDriver.hpp>

#include "Kernel.hpp"
#include "ThreadManager.hpp"

namespace Quantum::Kernel::Concurrency {
  /**
    * @brief Entry point for the idle @ref Thread.
    * @return Never returns.
    * @note Even though this function never returns, the signature notates an
    *       @ref Int32 return value to match the expected ABI for @ref Thread
    *       entry points.
    */
  inline Int32 IdleThread(void* argument) {
    ICPUDriver* cpu = static_cast<ICPUDriver*>(argument);

    for (;;) {
      cpu->Halt();
    }

    __builtin_unreachable();
  }
}
