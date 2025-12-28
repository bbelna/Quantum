/**
 * @file System/Kernel/Devices/DeviceManager.cpp
 * @brief Kernel device manager.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Devices/BlockDevices.hpp"
#include "Devices/DeviceManager.hpp"
#include "Devices/InputDevices.hpp"

namespace Quantum::System::Kernel::Devices {
  void DeviceManager::Initialize() {
    using namespace Devices;

    BlockDevices::Initialize();
    InputDevices::Initialize();
  }
}
