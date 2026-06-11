/**
 * @file Kernel/Memory/MemoryMappingFlags.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryMappingFlags.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "MemoryMappingPermissions.hpp"
#include "MemoryMappingCache.hpp"
#include "MemoryMappingOptions.hpp"

namespace Quantum::Kernel::Memory {
  /**
   * @brief Flags for mapping @ref MemoryBlock.
   */
  struct MemoryMappingFlags {
    /**
     * @brief Permissions for the mapped @ref MemoryBlock.
     */
    MemoryMappingPermissions Permissions;

    /**
     * @brief Caching strategy for the mapped @ref MemoryBlock.
     */
    MemoryMappingCache Cache;

    /**
     * @brief Additional mapping options for the mapped @ref MemoryBlock.
     */
    MemoryMappingOptions Options;
  };
}
