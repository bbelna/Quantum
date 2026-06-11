/**
 * @file Kernel/Arch/IA32/Memory/IA32StackManagerConfigurationProvider.cpp
 * @brief Implements @ref @QKrnlIA32::Memory::IA32StackManagerConfigurationProvider.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <Arch/IA32/IA32Constants.hpp>

#include "IA32StackManagerConfigurationProvider.hpp"

namespace Quantum::Kernel::Arch::IA32::Memory {
  StackManagerConfiguration IA32StackManagerConfigurationProvider::Provide() {
    return StackManagerConfiguration {
      .Block = MemoryBlock {
        {
          STACK_BASE,
          STACK_SIZE_IN_BYTES
        }
      },
      .GuardBlockCount = STACK_GUARD_PAGE_COUNT
    };
  }
}
