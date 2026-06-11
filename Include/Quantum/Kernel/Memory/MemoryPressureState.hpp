/**
 * @file Include/Quantum/Kernel/Memory/MemoryPressureState.hpp
 * @brief Declares @ref @QKrnl::Memory::MemoryPressureState.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Memory {
  /**
   * @brief Memory pressure states.
   */
  enum class MemoryPressureState : UInt8 {
    /**
     * @brief Memory usage is within normal operating bounds.
     */
    Normal = 0,

    /**
     * @brief Memory usage is elevated.
     */
    Elevated = 1,

    /**
     * @brief Memory is critically low.
     */
    Critical = 2
  };
}
