/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Include/Service.hpp
 * FAT12 file system service.
 */

#pragma once

#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12 {

  /**
   * FAT12 file system service.
   */
  class Service {
    public:
      /**
       * Entry point for the FAT12 service.
       */
      static void Main();

    private:
      /**
       * Mounted FAT12 volume state.
       */
      inline static Volume* _volume = nullptr;

      /**
       * Storage for the FAT12 volume instance.
       */
      inline static Volume _volumeStorage {};

      /**
       * Initializes the FAT12 volume cache.
       */
      static void InitializeVolume();
  };
}
