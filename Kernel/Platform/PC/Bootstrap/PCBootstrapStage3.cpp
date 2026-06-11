/**
 * @file Kernel/Platform/PC/Bootstrap/PCBootstrapStage3.cpp
 * @brief Implements @ref @QKrnlPC::Bootstrap::PCBootstrapStage3.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "PCKernelInitializer.hpp"
#include "PCBootstrapStage3.hpp"

namespace Quantum::Kernel::Platform::PC::Bootstrap {
  void PCBootstrapStage3() {
    Kernel& kernel = Kernel::Instance();
    static PCKernelInitializer initializer;

    kernel.Initialize(
      &initializer,
      InitialProcessInfo {
        MemoryBlock {
          BootInfo->InitialImagePhysicalAddress,
          BootInfo->InitialImageSizeInBytes
        },
        BootInfo->InitialProcessEntryPointOffset,
        BootInfo->InitialProcessImageSize,
        BootInfo->InitialProcessName
      }
    );

    __builtin_unreachable();
  }
}
