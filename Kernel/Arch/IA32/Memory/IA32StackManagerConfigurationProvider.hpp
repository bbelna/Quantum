/**
 * @file
 *   Kernel/Arch/IA32/Memory/IA32StackManagerConfigurationProvider.hpp
 * @brief
 *   Declares @ref @QKrnlIA32::Memory::IA32StackManagerConfigurationProvider.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Memory/StackManagerConfiguration.hpp>

namespace Quantum::Kernel::Arch::IA32::Memory {
  /**
   * @brief Provides the IA-32 stack manager configuration.
   *
   * Returns a @ref StackManagerConfiguration with the architecture-specific
   * stack region base address, size, and guard page count derived from the
   * IA-32 memory layout constants.
   */
  class IA32StackManagerConfigurationProvider : public IProvider<
    StackManagerConfiguration
  > {
    public:
      /**
       * @brief Provides the stack manager configuration.
       * @return The IA-32 stack manager configuration.
       */
      StackManagerConfiguration Provide() override;
  };
}
