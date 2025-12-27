/**
 * @file System/Coordinator/Include/Application.hpp
 * @brief System coordinator application.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <ABI/InitBundle.hpp>
#include <Types.hpp>

namespace Quantum::System::Coordinator {
  using InitBundle = ABI::InitBundle;

  class Application {
    public:
      /**
       * Coordinator entry point. Reads INIT.BND and spawns entries.
       * Driver entries are granted I/O access.
       */
      static void Main();

    private:
      /**
       * Device type identifiers for startup dependencies.
       */
      enum class DeviceType : UInt8 {
        None = 0,
        Floppy = 1
      };

      /**
       * INIT.BND header layout.
       */
      using BundleHeader = InitBundle::Header;

      /**
       * INIT.BND entry layout.
       */
      using BundleEntry = InitBundle::Entry;

      /**
       * Detected device bitmask.
       */
      inline static UInt8 _detectedDevices = 0;

      /**
       * Spawned device bitmask.
       */
      inline static UInt8 _spawnedDevices = 0;

      /**
       * Ready device bitmask.
       */
      inline static UInt8 _readyDevices = 0;

      /**
       * Coordinator readiness port id.
       */
      inline static UInt32 _readyPortId = 0;

      /**
       * Maximum INIT.BND entries to process.
       */
      static constexpr UInt32 _maxBundleEntries = 64;

      /**
       * Validates the INIT.BND header magic.
       * @param header
       *   INIT.BND header to validate.
       * @return
       *   True if the magic is valid; false otherwise.
       */
      static bool HasMagic(const BundleHeader& header);

      /**
       * Returns the length of the entry name.
       * @param entry
       *   Entry to measure.
       * @return
       *   Length of the entry name.
       */
      static UInt32 EntryNameLength(const BundleEntry& entry);

      /**
       * Compares an entry name to a target name.
       * @param entry
       *   Entry to compare.
       * @param name
       *   Null-terminated target name.
       * @return
       *   True if the names are equal; false otherwise.
       */
      static bool EntryNameEquals(const BundleEntry& entry, CString name);

      /**
       * Spawns an INIT.BND entry.
       * @param entry
       *   INIT.BND entry to spawn.
       * @return
       *   True if spawned successfully; false otherwise.
       */
      static bool SpawnEntry(const BundleEntry& entry);

      /**
       * Detects available devices.
       * @return
       *   Detected device mask.
       */
      static UInt8 DetectDevices();

      /**
       * Returns the device mask for a device identifier.
       * @param deviceId
       *   Device identifier from INIT.BND.
       * @return
       *   Device mask (0 if invalid).
       */
      static UInt8 DeviceMaskFromId(UInt8 deviceId);

      /**
       * Reads a CMOS register.
       * @param reg
       *   CMOS register index.
       * @return
       *   Register value.
       */
      static UInt8 ReadCMOS(UInt8 reg);

      /**
       * Detects whether a floppy device is present.
       * @return
       *   True if a floppy device is present; false otherwise.
       */
      static bool HasFloppyDevice();

      /**
       * Processes pending readiness messages.
       */
      static void ProcessReadyMessages();

  };
}
