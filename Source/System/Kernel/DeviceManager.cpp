/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/DeviceManager.cpp
 * Kernel device manager.
 */

#include "DeviceManager.hpp"
#include "Devices/BlockDevice.hpp"

namespace Quantum::System::Kernel {
  void DeviceManager::Initialize() {
    using namespace Devices;

    BlockDevice::Initialize();
  }
}
