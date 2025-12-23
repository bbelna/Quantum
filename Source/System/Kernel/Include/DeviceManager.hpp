/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/DeviceManager.hpp
 * Kernel device manager.
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
