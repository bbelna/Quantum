/**
 * @file Kernel/Resources/KernelResourcesModule.cpp
 * @brief Implements @ref @QKrnl::Resources::KernelResourcesModule.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <KernelContext.hpp>

#include "KernelResourceManager.hpp"
#include "KernelResourcesModule.hpp"

namespace Quantum::Kernel::Resources {
  bool KernelResourcesModule::Initialize(KernelContext* context) {
    static KernelResourceManager resourceManager;

    context->KernelResourceManager = &resourceManager;

    return true;
  }
}
