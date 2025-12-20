/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.hpp
 * User-mode floppy driver entry.
 */

#pragma once

namespace Quantum::System::Drivers::Storage::Floppy {
  /**
   * Floppy driver.
   */
  class Driver {
    public:
      /**
       * Entry point for the floppy driver service.
       */
      static void Main();
  };
}
