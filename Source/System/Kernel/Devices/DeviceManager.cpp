/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Devices/DeviceManager.cpp
 * Kernel device manager.
 */

#include "Devices/BlockDevice.hpp"
#include "Devices/DeviceManager.hpp"

namespace Quantum::System::Kernel::Devices {
  void DeviceManager::Initialize() {
    BlockDevice::Initialize();
  }
}
