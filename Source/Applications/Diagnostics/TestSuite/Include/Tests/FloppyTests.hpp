/**
 * @file Applications/Diagnostics/TestSuite/Include/Tests/FloppyTests.hpp
 * @brief Floppy block device tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/Devices/BlockDevices.hpp>
#include <Types.hpp>

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  /**
   * Floppy block device tests.
   */
  class FloppyTests {
    public:
      /**
       * Registers floppy tests with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Indicates whether a skip reason has been logged.
       */
      inline static bool _skipLogged = false;

      /**
       * Logs a skip reason.
       * @param reason
       *   Skip reason.
       */
      static void LogSkip(CString reason);

      /**
       * Finds a ready floppy device.
       * @param outId
       *   Output device identifier.
       * @param outInfo
       *   Output device info.
       * @return
       *   True if a ready floppy device was found.
       */
      static bool FindFloppyDevice(
        UInt32& outToken,
        ABI::Devices::BlockDevices::Info& outInfo
      );

      /**
       * Closes a device token if it is a handle.
       * @param deviceToken
       *   Device token to close.
       * @param deviceId
       *   Raw device identifier.
       */
      static void CloseDeviceToken(UInt32 deviceToken, UInt32 deviceId);

      /**
       * Reads sectors from the floppy device.
       * @param deviceId
       *   Device identifier.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to read.
       * @param buffer
       *   Output buffer.
       * @return
       *   True on success.
       */
      static bool ReadSectors(
        UInt32 deviceToken,
        UInt32 lba,
        UInt32 count,
        void* buffer
      );

      /**
       * Writes sectors to the floppy device.
       * @param deviceId
       *   Device identifier.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to write.
       * @param buffer
       *   Input buffer.
       * @return
       *   True on success.
       */
      static bool WriteSectors(
        UInt32 deviceToken,
        UInt32 lba,
        UInt32 count,
        void* buffer
      );

      /**
       * Tests single-sector read functionality.
       * @return
       *   True on success.
       */
      static bool TestSingleSectorRead();

      /**
       * Tests multi-sector read functionality.
       * @return
       *   True on success.
       */
      static bool TestMultiSectorRead();

      /**
       * Tests write and readback functionality.
       * @return
       *   True on success.
       */
      static bool TestWriteReadback();

      /**
       * Tests multi-sector write and readback functionality.
       * @return
       *   True on success.
       */
      static bool TestMultiSectorWriteReadback();

      /**
       * Tests cross-track write and readback functionality.
       * @return
       *   True on success.
       */
      static bool TestCrossTrackWriteReadback();
  };
}
