/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/BootInfo.cpp
 * IA32 boot info parsing and architecture-neutral accessors.
 */

#include <Arch/IA32/BootInfo.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Arch::IA32 {
  [[gnu::section(".text.start.data")]]
  BootInfo::View BootInfo::_bootInfoView{};

  [[gnu::section(".text.start.data")]]
  UInt32 BootInfo::_bootInfoPhysicalAddress = 0;

  [[gnu::section(".text.start.data")]]
  UInt32 BootInfo::_bootInfoValid = 0;

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
