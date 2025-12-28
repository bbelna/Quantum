/**
 * @file System/Kernel/BootInfo.cpp
 * @brief Architecture-agnostic boot info handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <Types.hpp>

#include "Arch/BootInfo.hpp"
#include "BootInfo.hpp"

namespace Quantum::System::Kernel {
  void BootInfo::Initialize(UInt32 bootInfoPhysicalAddress) {
    Arch::BootInfo::Initialize(bootInfoPhysicalAddress);
  }

  bool BootInfo::GetInitBundleInfo(InitBundleInfo& info) {
    const Arch::BootInfo::View* view = Arch::BootInfo::Get();

    if (!view || view->initBundleSize == 0) {
      info.physical = 0;
      info.size = 0;

      return false;
    }

    info.physical = view->initBundlePhysical;
    info.size = view->initBundleSize;

    return true;
  }
}
