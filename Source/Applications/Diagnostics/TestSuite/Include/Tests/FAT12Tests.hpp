/**
 * @file Applications/Diagnostics/TestSuite/Include/Tests/FAT12Tests.hpp
 * @brief FAT12 file system tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/FileSystem.hpp>
#include <Types.hpp>

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  /**
   * FAT12 file system tests.
   */
  class FAT12Tests {
    public:
      /**
       * Registers FAT12 tests with the harness.
       */
      static void RegisterTests();

    private:
      /**
       * Indicates whether a skip reason has been logged.
       */
      inline static bool _skipLogged = false;

      /**
       * Long file name test directory.
       */
      inline static const char _lfnDir[] = "LONGDIRNAME";

      /**
       * Long file name test file.
       */
      inline static const char _lfnFile[] = "LONGFILENAME.TXT";

      /**
       * Test directory.
       */
      inline static const char _testDir[] = "TESTDIR";

      /**
       * Test file.
       */
      inline static const char _testFile[] = "TEST.TXT";

      /**
       * Append test file.
       */
      inline static const char _appendFile[] = "APPEND.TXT";

      /**
       * Logs a skip reason.
       * @param reason
       *   Skip reason.
       */
      static void LogSkip(CString reason);

      /**
       * Compares two names for equality.
       * @param left
       *   Left name.
       * @param right
       *   Right name.
       * @return
       *   True if names are equal.
       */
      static bool NameEquals(CString left, CString right);

      /**
       * Checks whether a directory entry has a valid timestamp.
       * @param entry
       *   Directory entry.
       * @return
       *   True if the entry has a valid timestamp.
       */
      static bool HasTimestamp(const ABI::FileSystem::DirectoryEntry& entry);

      /**
       * Waits for a floppy device to become ready.
       * @return
       *   True if a floppy device is ready.
       */
      static bool WaitForFloppyReady();

      /**
       * Opens the FAT12 volume.
       * @param outVolume
       *   Output volume handle.
       * @return
       *   True on success.
       */
      static bool OpenFat12Volume(ABI::FileSystem::VolumeHandle& outVolume);

      /**
       * Opens a path on the given volume.
       * @param volume
       *   Volume handle.
       * @param path
       *   Path to open.
       * @param outHandle
       *   Output file handle.
       * @param outInfo
       *   Output file info.
       * @return
       *   True on success.
       */
      static bool OpenPath(
        ABI::FileSystem::VolumeHandle volume,
        CString path,
        ABI::FileSystem::Handle& outHandle,
        ABI::FileSystem::FileInfo& outInfo
      );

      /**
       * Ensures a directory exists on the given volume.
       * @param volume
       *   Volume handle.
       * @param path
       *   Directory path.
       * @return
       *   True on success.
       */
      static bool EnsureDirectory(
        ABI::FileSystem::VolumeHandle volume,
        CString path
      );

      /**
       * Ensures a file exists on the given volume.
       * @param volume
       *   Volume handle.
       * @param path
       *   File path.
       * @return
       *   True on success.
       */
      static bool EnsureFile(
        ABI::FileSystem::VolumeHandle volume,
        CString path
      );

      /**
       * Writes contents to a file on the given volume.
       * @param volume
       *   Volume handle.
       * @param path
       *   File path.
       * @param data
       *   Input data.
       * @param length
       *   Data length in bytes.
       * @return
       *   True on success.
       */
      static bool WriteFileContents(
        ABI::FileSystem::VolumeHandle volume,
        CString path,
        const UInt8* data,
        UInt32 length
      );

      /**
       * Cleans up test entries on the FAT12 volume.
       */
      static void CleanupTestEntries();

      /**
       * FAT12 volume load test.
       * @return
       *   True on success.
       */
      static bool TestVolumeLoad();

      /**
       * FAT12 volume info test.
       * @return
       *   True on success.
       */
      static bool TestVolumeInfo();

      /**
       * FAT12 root directory test.
       * @return
       *   True on success.
       */
      static bool TestRootDirectory();

      /**
       * FAT12 sub-directory test.
       * @return
       *   True on success.
       */
      static bool TestSubDirectory();

      /**
       * FAT12 long file name root test.
       * @return
       *   True on success.
       */
      static bool TestLFNRoot();

      /**
       * FAT12 long file name file test.
       * @return
       *   True on success.
       */
      static bool TestLFNFile();

      /**
       * FAT12 file read test.
       * @return
       *   True on success.
       */
      static bool TestFileRead();

      /**
       * FAT12 file seek test.
       * @return
       *   True on success.
       */
      static bool TestFileSeek();

      /**
       * FAT12 file write append test.
       * @return
       *   True on success.
       */
      static bool TestFileWriteAppend();

      /**
       * FAT12 create directory test.
       * @return
       *   True on success.
       */
      static bool TestCreateDirectory();

      /**
       * FAT12 create file test.
       * @return
       *   True on success.
       */
      static bool TestCreateFile();

      /**
       * FAT12 stat test.
       * @return
       *   True on success.
       */
      static bool TestStat();

      /**
       * FAT12 rename test.
       * @return
       *   True on success.
       */
      static bool TestRename();

      /**
       * FAT12 remove test.
       * @return
       *   True on success.
       */
      static bool TestRemove();

      /**
       * FAT12 path normalization test.
       * @return
       *   True on success.
       */
      static bool TestPathNormalization();

      /**
       * FAT12 cleanup test.
       * @return
       *   True on success.
       */
      static bool TestCleanup();
  };
}
