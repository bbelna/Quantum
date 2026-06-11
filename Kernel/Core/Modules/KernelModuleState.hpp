/**
 * @file Kernel/Core/KernelModuleState.hpp
 * @brief Declares @ref @QKrnl::Core::KernelModuleState.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Core {
  /**
   * @brief Lifecycle state of a kernel module.
   */
  enum class KernelModuleState : UInt8 {
    /**
     * @brief Module is registered but not yet initialized.
     */
    Registered,

    /**
     * @brief Module has been successfully initialized.
     */
    Initialized,

    /**
     * @brief Module initialization failed.
     */
    Failed
  };
}
