/**
 * @file Kernel/Core/IKernelInitializer.hpp
 * @brief Declares @ref @QKrnl::Core::IKernelInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <KernelContext.hpp>

namespace Quantum::Kernel::Core {
  /**
   * @brief Abstract interface for kernel initializers.
   *
   * Concrete implementations (e.g., @ref PCKernelInitializer) perform all
   * kernel initialization and populate the @ref KernelContext with the
   * resulting subsystem pointers. Called by @ref Kernel::Initialize as the
   * first step of kernel startup.
   */
  class IKernelInitializer : public IInitializer<KernelContext*, void> {};
}
