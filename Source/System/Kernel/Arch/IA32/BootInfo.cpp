/**
 * @file System/Kernel/Arch/IA32/BootInfo.cpp
 * @brief IA32 boot info handling.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include <Types.hpp>

#include "Arch/IA32/BootInfo.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  void BootInfo::Initialize(UInt32 bootInfoPhysicalAddress) {
    _bootInfoPhysicalAddress = bootInfoPhysicalAddress;
    _bootInfoValid = 0;
    _bootInfoView = {};

    if (bootInfoPhysicalAddress == 0) {
      return;
    }

    const Raw* raw = reinterpret_cast<const Raw*>(bootInfoPhysicalAddress);

    _bootInfoView.entryCount = raw->entryCount;
    _bootInfoView.reserved = raw->reserved;
    _bootInfoView.initBundlePhysical = raw->initBundlePhysical;
    _bootInfoView.initBundleSize = raw->initBundleSize;

    UInt32 count = raw->entryCount;
    if (count > maxEntries) {
      count = maxEntries;
    }

    for (UInt32 i = 0; i < count; ++i) {
      _bootInfoView.entries[i] = raw->entries[i];
    }

    _bootInfoValid = 1;
  }

  const BootInfo::View* BootInfo::Get() {
    if (_bootInfoValid == 0) {
      return nullptr;
    }

    return &_bootInfoView;
  }

  UInt32 BootInfo::GetPhysicalAddress() {
    return _bootInfoPhysicalAddress;
  }
}
