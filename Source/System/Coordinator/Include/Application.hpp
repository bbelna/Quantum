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
  using InitBundle = ABI::InitBundle;

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
      using BundleHeader = InitBundle::Header;

      /**
       * INIT.BND entry layout.
       */
      using BundleEntry = InitBundle::Entry;

      /**
       * Cached floppy presence state.
       */
      inline static bool _floppyPresent = false;

      /**
       * Cached floppy block device id.
       */
      inline static UInt32 _floppyDeviceId = 0;

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
       */
      static void SpawnEntry(const BundleEntry& entry);

      /**
       * Detects whether a floppy block device is present.
       * @return
       *   True if a floppy device is present; false otherwise.
       */
      static bool HasFloppyDevice();

      /**
       * Runs a small block read test against the floppy device.
       */
      static void TestFloppyBlockRead();

      /**
       * Runs a multi-sector block read test against the floppy device.
       */
      static void TestFloppyMultiSectorRead();

      /**
       * Runs a single-sector write and read-back test.
       */
      static void TestFloppyWriteReadback();

      /**
       * Runs a multi-sector write and read-back test.
       */
      static void TestFloppyMultiSectorWriteReadback();

      /**
       * Runs a cross-track multi-sector write and read-back test.
       */
      static void TestFloppyCrossTrackWriteReadback();
  };
}
