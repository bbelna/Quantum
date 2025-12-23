/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/DeviceManager.cpp
 * Kernel device manager.
 */

#include "DeviceManager.hpp"
#include "Devices/BlockDevice.hpp"

#if defined(QUANTUM_ARCH_IA32)
#include "Arch/IA32/Floppy.hpp"
#endif

namespace Quantum::System::Kernel {
  void DeviceManager::Initialize() {
    using namespace Devices;

    BlockDevice::Initialize();

    #if defined(QUANTUM_ARCH_IA32)
    Arch::IA32::Floppy::Initialize();
    #endif
  }
}
