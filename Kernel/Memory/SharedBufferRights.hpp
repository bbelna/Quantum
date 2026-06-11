/**
 * @file Kernel/Memory/SharedBufferRights.hpp
 * @brief Declares @ref @QKrnl::Memory::SharedBufferRights.
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
   * @brief Rights associated with a @ref SharedBufferResource.
   */
  enum class SharedBufferRights : UInt32 {
    /**
     * @brief Indicates the @ref Process created the @ref SharedBuffer.
     */
    Owner  = 1u << 0,

    /**
     * @brief Indicates the @ref Process has an active mapping of the
     *        @ref SharedBuffer.
     */
    Attach = 1u << 1,
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Kernel::Memory,
  SharedBufferRights
)
