/**
 * @file System/Kernel/Include/DeviceManager.hpp
 * @brief Kernel device manager.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace Quantum::System::Kernel {
  /**
   * Kernel device manager that initializes device registries.
   */
  class DeviceManager {
    public:
      /**
       * Initializes device registries and probes hardware.
       */
      static void Initialize();
  };
}
