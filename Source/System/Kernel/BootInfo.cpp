/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/BootInfo.cpp
 * Kernel boot info accessors.
 */

#include <BootInfo.hpp>
#include <Types.hpp>

#if defined(QUANTUM_ARCH_IA32)
#include <Arch/IA32/BootInfo.hpp>
#endif

namespace Quantum::System::Kernel {
  void BootInfo::Initialize(UInt32 bootInfoPhysicalAddress) {
    #if defined(QUANTUM_ARCH_IA32)
    Arch::IA32::BootInfo::Initialize(bootInfoPhysicalAddress);
    #else
    (void)bootInfoPhysicalAddress;
    #endif
  }

  bool BootInfo::GetInitBundleInfo(InitBundleInfo& info) {
    #if defined(QUANTUM_ARCH_IA32)
    const Arch::IA32::BootInfo::View* view = Arch::IA32::BootInfo::Get();

    if (!view || view->initBundleSize == 0) {
      info.physical = 0;
      info.size = 0;
      return false;
    }

    info.physical = view->initBundlePhysical;
    info.size = view->initBundleSize;
    return true;
    #else
    info.physical = 0;
    info.size = 0;
    return false;
    #endif
  }
}
