/**
 * @file System/Kernel/Objects/Devices/BlockDeviceObject.cpp
 * @brief Block device kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Objects/Devices/BlockDeviceObject.hpp"

namespace Quantum::System::Kernel::Objects::Devices {
  BlockDeviceObject::BlockDeviceObject(UInt32 device)
    : KernelObject(KernelObjectType::BlockDevice),
    deviceId(device)
  {}
}
