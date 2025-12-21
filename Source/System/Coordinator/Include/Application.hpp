/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Coordinator.hpp
 * System coordinator entry declaration.
 */

#pragma once

#include <ABI/InitBundle.hpp>
#include <Types.hpp>

namespace Quantum::System::Coordinator {
  class Application {
    public:
      /**
       * Coordinator entry point.
       */
      static void Main();

    private:
      /**
       * INIT.BND header layout.
       */
      using BundleHeader = Quantum::ABI::InitBundle::Header;

      /**
       * INIT.BND entry layout.
       */
      using BundleEntry = Quantum::ABI::InitBundle::Entry;

      /**
       * Validates the INIT.BND header magic.
       */
      static bool HasMagic(const BundleHeader& header);

      /**
       * Returns the length of the entry name.
       */
      static UInt32 EntryNameLength(const BundleEntry& entry);

      /**
       * Compares an entry name to a target name.
       */
      static bool EntryNameEquals(const BundleEntry& entry, CString name);

      /**
       * Spawns an INIT.BND entry, skipping the coordinator.
       */
      static void SpawnEntry(const BundleEntry& entry);

      /**
       * Detects whether a floppy block device is present.
       */
      static bool HasFloppyDevice();

      /**
       * Runs a small block read test against the floppy device.
       */
      static void TestFloppyBlockRead();

      /**
       * Cached floppy presence state.
       */
      static bool _floppyPresent;

      /**
       * Cached floppy block device id.
       */
      static UInt32 _floppyDeviceId;
  };
}
