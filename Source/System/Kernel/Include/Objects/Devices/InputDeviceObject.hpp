/**
 * @file System/Kernel/Include/Objects/Devices/InputDeviceObject.hpp
 * @brief Input device kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/KernelObject.hpp"

namespace Quantum::System::Kernel::Objects::Devices {
  /**
   * Input device kernel object.
   */
  class InputDeviceObject : public KernelObject {
    public:
      /**
       * Constructs an input device object.
       * @param device
       *   Input device identifier.
       */
      explicit InputDeviceObject(UInt32 device);

      /**
       * Input device identifier.
       */
      UInt32 deviceId;
  };
}
