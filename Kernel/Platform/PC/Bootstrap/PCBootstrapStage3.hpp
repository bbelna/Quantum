/**
 * @file Kernel/Platform/PC/Bootstrap/PCBootstrapStage3.hpp
 * @brief Declares @ref @QKrnlPC::Bootstrap::PCBootstrapStage3.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel::Platform::PC::Bootstrap {
  /**
   * @brief Stage 3 of the kernel bootstrap process for the PC platform.
   *
   * Called by @ref @QKrnlIA32::Bootstrap::IA32BootstrapStage2 after the initial
   * image has been relocated and global constructors have run. This is the
   * platform-specific entry point for full kernel initialization.
   *
   * @ref PCBootstrapStage3 performs the following steps, in order:
   *
   *   1. Constructs a @ref PCKernelInitializer, which configures all
   *      PC-specific hardware subsystems.
   *
   *   2. Calls @ref Kernel::Initialize with the platform initializer and
   *      an @ref InitialProcessInfo describing the initial image location
   *      and process metadata (sourced from @ref BootInfo), transferring
   *      control to the main kernel entry point.
   *
   * This function does not return.
   */
  [[noreturn]] void PCBootstrapStage3();
}
