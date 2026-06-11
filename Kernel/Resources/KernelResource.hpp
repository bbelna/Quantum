/**
 * @file Kernel/Resources/KernelResource.hpp
 * @brief Declares @ref @QKrnl::Resources::KernelResource.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "KernelResourceBase.hpp"

namespace Quantum::Kernel::Resources {
  /**
   * @brief Generic base kernel resource structure.
   * @tparam ObjectIDType Type of the object identifier (e.g., file
   *                      descriptor, memory region ID, etc.).
   * @tparam ResourceRightsType Type of the rights/permissions associated with
   *                            the allocated resource (default is `UInt32`).
   *
   * Contains a unique `ID` (@ref ResourceID) and the `ObjectID` that this
   * @ref KernelResource corresponds to.
   */
  template <typename ObjectIDType, typename ResourceRightsType = UInt32>
  struct KernelResource : public KernelResourceBase {
    /**
     * @brief ID of the kernel object that the @ref KernelResource
     *        corresponds to (e.g., file descriptor number, memory region ID,
     *        etc.).
     */
    ObjectIDType ObjectID;

    /**
     * @brief Rights associated with the @ref KernelResource.
     */
    ResourceRightsType Rights;
  };
}
