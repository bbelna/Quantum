/**
 * @file Kernel/Resources/KernelResourceType.hpp
 * @brief Declares @ref @QKrnl::Resources::KernelResourceType.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelTypes.hpp>

namespace Quantum::Kernel::Resources {
  /**
   * @brief Runtime type discriminator for kernel resources.
   */
  enum class KernelResourceType : UInt8 {
    /**
     * @brief IPC port resource type.
     */
    IPCPort = 0,

    /**
     * @brief Interrupt resource type.
     */
    Interrupt = 1,

    /**
     * @brief Display resource type.
     */
    Display = 2,

    /**
     * @brief Shared buffer resource type.
     */
    SharedBuffer = 3,
  };
}
