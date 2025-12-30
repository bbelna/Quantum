/**
 * @file System/Kernel/Objects/Devices/InputDeviceObject.cpp
 * @brief Input device kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "Objects/Devices/InputDeviceObject.hpp"

namespace Quantum::System::Kernel::Objects::Devices {
  InputDeviceObject::InputDeviceObject(UInt32 device)
    : KernelObject(KernelObjectType::InputDevice),
    deviceId(device)
  {}
}
