/**
 * @file System/Kernel/Include/Objects/Devices/BlockDeviceObject.hpp
 * @brief Block device kernel object.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/KernelObject.hpp"

namespace Quantum::System::Kernel::Objects::Devices {
  /**
   * Block device kernel object.
   */
  class BlockDeviceObject : public KernelObject {
    public:
      /**
       * Constructs a block device object.
       * @param device
       *   Block device identifier.
       */
      explicit BlockDeviceObject(UInt32 device);

      /**
       * Block device identifier.
       */
      UInt32 deviceId;
  };
}
