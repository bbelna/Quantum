/**
 * @file Kernel/Concurrency/ProcessPermissions.hpp
 * @brief Declares @ref @QKrnl::Concurrency::ProcessPermissions.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Concurrency {
  /**
   * @brief Permission flags for processes.
   */
  enum class ProcessPermissions : UInt32 {
    /**
     * @brief Process has no special permissions.
     */
    None = 0x0u,

    /**
     * @brief Process has permission to perform port I/O operations.
     */
    PortIO = 0x1u,

    /**
     * @brief Process has permission to handle interrupts.
     */
    Interrupts = 0x2u,
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Kernel::Concurrency,
  ProcessPermissions
)
