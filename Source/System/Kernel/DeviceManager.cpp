/**
 * @file System/Kernel/DeviceManager.cpp
 * @brief Kernel device manager.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "DeviceManager.hpp"
#include "Devices/BlockDevices.hpp"

namespace Quantum::System::Kernel {
  void DeviceManager::Initialize() {
    using namespace Devices;

    BlockDevices::Initialize();
  }
}
