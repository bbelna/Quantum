/**
 * @file Kernel/Memory/MemoryMappingPermissions.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryMappingPermissions.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Permissions for mapping @ref MemoryBlock.
   */
  enum class MemoryMappingPermissions : UInt32 {
    /**
     * @brief No permissions.
     */
    None = 0,

    /**
     * @brief Read permission.
     */
    Read = 1u << 0,

    /**
     * @brief Write permission.
     */
    Write = 1u << 1,

    /**
     * @brief Execute permission.
     */
    Execute = 1u << 2,

    /**
     * @brief User-mode access permission.
     */
    User = 1u << 3
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Kernel::Memory, MemoryMappingPermissions
)
