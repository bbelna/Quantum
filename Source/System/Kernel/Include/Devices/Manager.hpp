/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Devices/Manager.hpp
 * Kernel device manager.
 */

#pragma once

namespace Quantum::System::Kernel::Devices {
  /**
   * Kernel device manager that initializes device registries.
   */
  class Manager {
    public:
      /**
       * Initializes device registries and probes hardware.
       */
      static void Initialize();
  };
}
