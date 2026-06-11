/**
 * @file Kernel/Resources/KernelResourceBase.hpp
 * @brief Declares @ref @QKrnl::Resources::KernelResourceBase.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Concurrency/RefCountScoped.hpp>
#include <KernelTypes.hpp>

#include "KernelResourceType.hpp"

namespace Quantum::Kernel::Resources {
  /**
   * @brief Base non-templated structure for kernel resources.
   *
   * Provides fields that @ref ResourceManager needs to access without knowing
   * the concrete resource type.
   */
  struct KernelResourceBase : public RefCountScoped {
    /**
     * @brief The @ref ResourceID.
     */
    ResourceID ID;

    /**
     * @brief Runtime type discriminator for the resource.
     */
    KernelResourceType Type;

    /**
     * @brief @ref ProcessID of the @ref Process that owns the resource.
     */
    ProcessID OwnerPID;
  };
}
