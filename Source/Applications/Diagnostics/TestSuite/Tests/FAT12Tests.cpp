/**
 * @file Applications/Diagnostics/TestSuite/Tests/FAT12Tests.cpp
 * @brief FAT12 file system tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/FileSystem.hpp>
#include <ABI/Task.hpp>

#include "Testing.hpp"
#include "Tests/FAT12Tests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  using ABI::Console;
  using ABI::Devices::BlockDevices;
  using ABI::FileSystem;
  using ABI::Task;

  void FAT12Tests::LogSkip(CString reason) {
    if (_skipLogged) {
      return;
    }

    Console::Write("FAT12 tests skipped (");
    Console::Write(reason ? reason : "unknown");
    Console::WriteLine(")");

    _skipLogged = true;
  }

  bool FAT12Tests::NameEquals(CString left, CString right) {
    if (!left || !right) {
      return false;
    }

    UInt32 index = 0;

    while (left[index] != '\0' && right[index] != '\0') {
      if (left[index] != right[index]) {
        return false;
      }

      ++index;
    }

    return left[index] == '\0' && right[index] == '\0';
  }

  bool FAT12Tests::HasTimestamp(const FileSystem::DirectoryEntry& entry) {
    return entry.createDate != 0
      || entry.createTime != 0
      || entry.accessDate != 0
      || entry.writeDate != 0
      || entry.writeTime != 0;
  }

  bool FAT12Tests::WaitForFloppyReady() {
    for (UInt32 attempt = 0; attempt < 256; ++attempt) {
      UInt32 count = BlockDevices::GetCount();

      for (UInt32 i = 1; i <= count; ++i) {
        BlockDevices::Info info {};

        if (BlockDevices::GetInfo(i, info) != 0) {
          continue;
        }

        if (info.type != BlockDevices::Type::Floppy) {
          continue;
        }

      if (
        (info.flags & BlockDevices::flagReady) != 0
        && info.sectorSize != 0
        && info.sectorCount != 0
      ) {
        return true;
      }
    }

    Task::SleepTicks(1);
  }

    return false;
  }

  bool FAT12Tests::OpenVolume(FileSystem::VolumeHandle& outVolume) {
    FileSystem::VolumeEntry entries[8] = {};

    for (UInt32 attempt = 0; attempt < 128; ++attempt) {
      UInt32 count = FileSystem::ListVolumes(entries, 8);

      for (UInt32 i = 0; i < count && i < 8; ++i) {
        if (entries[i].fsType != static_cast<UInt32>(FileSystem::Type::FAT12)) {
          continue;
        }

        if (entries[i].label[0] == '\0') {
          continue;
        }

        FileSystem::VolumeHandle handle
          = FileSystem::OpenVolume(entries[i].label);

        if (handle != 0) {
          outVolume = handle;

          return true;
        }
      }

      Task::SleepTicks(1);
    }

    return false;
  }

  bool FAT12Tests::OpenPath(
    FileSystem::VolumeHandle volume,
    CString path,
    FileSystem::Handle& outHandle,
    FileSystem::FileInfo& outInfo
  ) {
    FileSystem::Handle handle = FileSystem::Open(volume, path, 0);

    if (handle == 0) {
      return false;
    }

    if (FileSystem::Stat(handle, outInfo) != 0) {
      FileSystem::Close(handle);

      return false;
    }

    outHandle = handle;

    return true;
  }

  bool FAT12Tests::EnsureDirectory(FileSystem::VolumeHandle volume, CString path) {
    FileSystem::Handle handle = 0;
    FileSystem::FileInfo info {};

    if (OpenPath(volume, path, handle, info)) {
      bool isDir = (info.attributes & 0x10) != 0;

      FileSystem::Close(handle);

      return isDir;
    }

    if (FileSystem::CreateDirectory(volume, path) != 0) {
      return false;
    }

    if (OpenPath(volume, path, handle, info)) {
      bool isDir = (info.attributes & 0x10) != 0;

      FileSystem::Close(handle);

      return isDir;
    }

    return false;
  }

  bool FAT12Tests::EnsureFile(FileSystem::VolumeHandle volume, CString path) {
    FileSystem::Handle handle = 0;
    FileSystem::FileInfo info {};

    if (OpenPath(volume, path, handle, info)) {
      bool isDir = (info.attributes & 0x10) != 0;

      FileSystem::Close(handle);

      return !isDir;
    }

    if (FileSystem::CreateFile(volume, path) != 0) {
      return false;
    }

    if (OpenPath(volume, path, handle, info)) {
      bool isDir = (info.attributes & 0x10) != 0;

      FileSystem::Close(handle);

      return !isDir;
    }

    return false;
  }

  bool FAT12Tests::WriteFileContents(
    FileSystem::VolumeHandle volume,
    CString path,
    const UInt8* data,
    UInt32 length
  ) {
    if (!data || length == 0) {
      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(volume, path, 0);

    if (handle == 0) {
      return false;
    }

    FileSystem::Seek(handle, 0, 0);

    UInt32 written = FileSystem::Write(handle, data, length);

    FileSystem::Seek(handle, 0, 0);
    FileSystem::Close(handle);

    return written == length;
  }

  void FAT12Tests::CleanupTestEntries() {
    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      return;
    }

    FileSystem::Remove(volume, "TESTDIR/TEST.TXT");
    FileSystem::Remove(volume, "TESTDIR/APPEND.TXT");
    FileSystem::Remove(volume, "TESTDIR");

    FileSystem::Remove(volume, "LONGDIRNAME/LONGFILENAME.TXT");
    FileSystem::Remove(volume, "LONGDIRNAME");

    FileSystem::Remove(volume, "NEWDIR/RENAMED.TXT");
    FileSystem::Remove(volume, "NEWDIR/NEWFILE.TXT");
    FileSystem::Remove(volume, "NEWDIR");

    FileSystem::CloseVolume(volume);
  }

  bool FAT12Tests::TestVolumeLoad() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    FileSystem::VolumeInfo info {};
    bool ok = FileSystem::GetVolumeInfo(volume, info) == 0;

    FileSystem::CloseVolume(volume);

    TEST_ASSERT(ok, "volume load failed");

    return ok;
  }

  bool FAT12Tests::TestVolumeInfo() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    FileSystem::VolumeInfo info {};
    bool ok = FileSystem::GetVolumeInfo(volume, info) == 0;

    if (!ok) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "volume info failed");

      return false;
    }

    ok &= TEST_ASSERT(
      info.fsType == static_cast<UInt32>(FileSystem::Type::FAT12),
      "volume type mismatch"
    );
    ok &= TEST_ASSERT(info.sectorSize != 0, "volume sector size missing");
    ok &= TEST_ASSERT(info.sectorCount != 0, "volume sector count missing");

    FileSystem::CloseVolume(volume);

    return ok;
  }

  bool FAT12Tests::TestRootDirectory() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    FileSystem::Handle root = FileSystem::Open(volume, "/", 0);

    if (root == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "root open failed");

      return false;
    }

    bool found = false;
    FileSystem::DirectoryEntry entry {};

    Console::WriteLine("FAT12 root directory entries:");

    for (;;) {
      if (FileSystem::ReadDirectory(root, entry) != 0) {
        break;
      }

      if (entry.name[0] == '\0') {
        break;
      }

      Console::Write("  ");
      Console::WriteLine(entry.name);

      found = true;
    }

    FileSystem::Close(root);
    FileSystem::CloseVolume(volume);

    if (!found) {
      LogSkip("no directory entries");

      return true;
    }

    TEST_ASSERT(true, "root entry read");

    return true;
  }

  bool FAT12Tests::TestSubDirectory() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, _testDir)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "testdir missing");

      return false;
    }

    FileSystem::Handle dirHandle = FileSystem::Open(volume, _testDir, 0);

    if (dirHandle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "testdir open failed");

      return false;
    }

    bool found = false;
    FileSystem::DirectoryEntry entry {};

    Console::WriteLine("FAT12 TESTDIR entries:");

    for (;;) {
      if (FileSystem::ReadDirectory(dirHandle, entry) != 0) {
        break;
      }

      if (entry.name[0] == '\0') {
        break;
      }

      Console::Write("  ");
      Console::WriteLine(entry.name);
      found = true;
    }

    FileSystem::Close(dirHandle);
    FileSystem::CloseVolume(volume);

    if (!found) {
      LogSkip("testdir empty");

      return true;
    }

    TEST_ASSERT(true, "testdir read");

    return true;
  }

  bool FAT12Tests::TestLFNRoot() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, _lfnDir)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "lfn dir create failed");

      return false;
    }

    FileSystem::Handle root = FileSystem::Open(volume, "/", 0);

    if (root == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "root open failed");

      return false;
    }

    bool found = false;
    FileSystem::DirectoryEntry entry {};

    for (;;) {
      if (FileSystem::ReadDirectory(root, entry) != 0) {
        break;
      }

      if (entry.name[0] == '\0') {
        break;
      }

      if (NameEquals(entry.name, _lfnDir)) {
        found = true;

        if ((entry.attributes & 0x10) == 0) {
          FileSystem::Close(root);
          FileSystem::CloseVolume(volume);

          TEST_ASSERT(false, "lfn dir not a directory");

          return false;
        }

        bool ok = TEST_ASSERT(HasTimestamp(entry), "lfn dir timestamp missing");

        FileSystem::Close(root);
        FileSystem::CloseVolume(volume);

        return ok;
      }
    }

    FileSystem::Close(root);
    FileSystem::CloseVolume(volume);

    TEST_ASSERT(found, "lfn dir missing");

    return found;
  }

  bool FAT12Tests::TestLFNFile() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, _lfnDir)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "lfn dir missing");

      return false;
    }

    const char lfnPath[] = "LONGDIRNAME/LONGFILENAME.TXT";

    if (!EnsureFile(volume, lfnPath)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "lfn file missing");

      return false;
    }

    FileSystem::Handle dir = FileSystem::Open(volume, _lfnDir, 0);

    if (dir == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "lfn dir open failed");

      return false;
    }

    FileSystem::DirectoryEntry entry {};

    for (;;) {
      if (FileSystem::ReadDirectory(dir, entry) != 0) {
        break;
      }

      if (entry.name[0] == '\0') {
        break;
      }

      if (NameEquals(entry.name, _lfnFile)) {
        if ((entry.attributes & 0x10) != 0) {
          FileSystem::Close(dir);
          FileSystem::CloseVolume(volume);

          TEST_ASSERT(false, "lfn file is a directory");

          return false;
        }

        bool ok = TEST_ASSERT(
          HasTimestamp(entry),
          "lfn file timestamp missing"
        );

        FileSystem::Close(dir);
        FileSystem::CloseVolume(volume);

        return ok;
      }
    }

    FileSystem::Close(dir);
    FileSystem::CloseVolume(volume);

    TEST_ASSERT(false, "lfn file missing");

    return false;
  }

  bool FAT12Tests::TestFileRead() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, _testDir)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "testdir missing");

      return false;
    }

    const char filePath[] = "TESTDIR/TEST.TXT";

    if (!EnsureFile(volume, filePath)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "test.txt missing");

      return false;
    }

    const char payload[] = "Quantum FAT12 test file.";

    if (
      !WriteFileContents(
        volume,
        filePath,
        reinterpret_cast<const UInt8*>(payload),
        static_cast<UInt32>(sizeof(payload) - 1)
      )
    ) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "test.txt write failed");

      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(volume, filePath, 0);

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "test.txt open failed");

      return false;
    }

    UInt8 buffer[128] = {};
    UInt32 bytesRead = FileSystem::Read(handle, buffer, sizeof(buffer) - 1);

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    if (bytesRead == 0) {
      TEST_ASSERT(false, "test.txt empty");

      return false;
    }

    buffer[bytesRead] = '\0';

    Console::WriteLine("FAT12 TEST.TXT contents:");
    Console::Write("  ");
    Console::WriteLine(reinterpret_cast<CString>(buffer));

    bool match = true;

    for (UInt32 i = 0; payload[i] != '\0'; ++i) {
      if (buffer[i] != static_cast<UInt8>(payload[i])) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "test.txt read");

    return match;
  }

  bool FAT12Tests::TestFileSeek() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    const char filePath[] = "TESTDIR/TEST.TXT";

    if (!EnsureDirectory(volume, _testDir) || !EnsureFile(volume, filePath)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "test.txt missing");

      return false;
    }

    const char payload[] = "Quantum FAT12 test file.";

    if (
      !WriteFileContents(
        volume,
        filePath,
        reinterpret_cast<const UInt8*>(payload),
        static_cast<UInt32>(sizeof(payload) - 1)
      )
    ) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "test.txt write failed");

      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(volume, filePath, 0);

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "test.txt open failed");

      return false;
    }

    UInt32 offset = FileSystem::Seek(handle, 8, 0);

    if (offset != 8) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "seek failed");

      return false;
    }

    char buffer[32] = {};
    UInt32 bytes = FileSystem::Read(handle, buffer, sizeof(buffer) - 1);

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    if (bytes == 0) {
      TEST_ASSERT(false, "seek read failed");

      return false;
    }

    buffer[bytes] = '\0';

    Console::WriteLine("FAT12 seek read:");
    Console::Write("  ");
    Console::WriteLine(buffer);

    const char expected[] = "FAT12";
    bool match = true;

    for (UInt32 i = 0; expected[i] != '\0'; ++i) {
      if (buffer[i] != expected[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "seek read");

    return match;
  }

  bool FAT12Tests::TestFileWriteAppend() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    const char filePath[] = "TESTDIR/APPEND.TXT";

    if (!EnsureDirectory(volume, _testDir) || !EnsureFile(volume, filePath)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "append file missing");

      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(volume, filePath, 0);

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "append file open failed");

      return false;
    }

    FileSystem::FileInfo info {};

    if (FileSystem::Stat(handle, info) != 0) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "append stat failed");

      return false;
    }

    UInt32 fileSize = info.sizeBytes;
    UInt32 offset = FileSystem::Seek(handle, 0, 2);

    if (offset != fileSize) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "append seek failed");

      return false;
    }

    const char appendText[] = "Quantum FAT12 append.\n";
    const UInt32 appendLength = sizeof(appendText) - 1;
    UInt32 written = FileSystem::Write(
      handle,
      appendText,
      appendLength
    );

    if (written != appendLength) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "append write failed");

      return false;
    }

    UInt32 readOffset = FileSystem::Seek(handle, fileSize, 0);

    if (readOffset != fileSize) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "append seek read failed");

      return false;
    }

    char verify[32] = {};
    UInt32 read = FileSystem::Read(handle, verify, appendLength);

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    if (read != appendLength) {
      TEST_ASSERT(false, "append read short");

      return false;
    }

    verify[appendLength] = '\0';

    bool match = true;

    for (UInt32 i = 0; i < appendLength; ++i) {
      if (verify[i] != appendText[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "append verify");

    return match;
  }

  bool FAT12Tests::TestCreateDirectory() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, "NEWDIR")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create directory failed");

      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(volume, "NEWDIR", 0);

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "directory not found");

      return false;
    }

    FileSystem::FileInfo info {};
    bool ok = FileSystem::Stat(handle, info) == 0;

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    if (!ok) {
      TEST_ASSERT(false, "directory stat failed");

      return false;
    }

    bool okAttr = (info.attributes & 0x10) != 0;

    TEST_ASSERT(okAttr, "directory not created");

    return okAttr;
  }

  bool FAT12Tests::TestCreateFile() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, "NEWDIR")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create directory failed");

      return false;
    }

    if (!EnsureFile(volume, "NEWDIR/NEWFILE.TXT")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create file failed");

      return false;
    }

    FileSystem::Handle handle
      = FileSystem::Open(volume, "NEWDIR/NEWFILE.TXT", 0);

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "file not found");

      return false;
    }

    FileSystem::FileInfo info {};
    bool ok = FileSystem::Stat(handle, info) == 0;

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    if (!ok) {
      TEST_ASSERT(false, "file stat failed");

      return false;
    }

    bool okAttr = (info.attributes & 0x10) == 0;

    TEST_ASSERT(okAttr, "file is a directory");

    return okAttr;
  }

  bool FAT12Tests::TestStat() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, "TESTDIR")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "stat missing");

      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(volume, "TESTDIR", 0);

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "stat open failed");

      return false;
    }

    FileSystem::FileInfo info {};
    bool ok = FileSystem::Stat(handle, info) == 0;

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    if (!ok) {
      TEST_ASSERT(false, "stat failed");

      return false;
    }

    ok &= TEST_ASSERT((info.attributes & 0x10) != 0, "stat attributes");
    ok &= TEST_ASSERT(info.sizeBytes == 0, "stat size");

    return ok;
  }

  bool FAT12Tests::TestRename() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, "NEWDIR")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create directory failed");

      return false;
    }

    if (!EnsureFile(volume, "NEWDIR/NEWFILE.TXT")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create file failed");

      return false;
    }

    if (FileSystem::Rename(
      volume,
      "NEWDIR/NEWFILE.TXT",
      "NEWDIR/RENAMED.TXT"
    ) != 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "rename failed");

      return false;
    }

    FileSystem::Handle oldHandle
      = FileSystem::Open(volume, "NEWDIR/NEWFILE.TXT", 0);
    bool oldExists = oldHandle != 0;

    if (oldHandle != 0) {
      FileSystem::Close(oldHandle);
    }

    FileSystem::Handle newHandle
      = FileSystem::Open(volume, "NEWDIR/RENAMED.TXT", 0);
    bool newExists = newHandle != 0;

    if (newHandle != 0) {
      FileSystem::Close(newHandle);
    }

    FileSystem::CloseVolume(volume);

    if (oldExists) {
      TEST_ASSERT(false, "old name still present");

      return false;
    }

    TEST_ASSERT(newExists, "renamed entry missing");

    return newExists;
  }

  bool FAT12Tests::TestRemove() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    if (!EnsureDirectory(volume, "NEWDIR")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create directory failed");

      return false;
    }

    if (!EnsureFile(volume, "NEWDIR/RENAMED.TXT")) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "create file failed");

      return false;
    }

    if (FileSystem::Remove(volume, "NEWDIR/RENAMED.TXT") != 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "remove file failed");

      return false;
    }

    FileSystem::Handle handle
      = FileSystem::Open(volume, "NEWDIR/RENAMED.TXT", 0);

    if (handle != 0) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "removed entry still present");

      return false;
    }

    if (FileSystem::Remove(volume, "NEWDIR") != 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "remove directory failed");

      return false;
    }

    handle = FileSystem::Open(volume, "NEWDIR", 0);

    if (handle != 0) {
      FileSystem::Close(handle);
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "directory still present");

      return false;
    }

    FileSystem::CloseVolume(volume);

    return true;
  }

  bool FAT12Tests::TestPathNormalization() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    FileSystem::VolumeHandle volume = 0;

    if (!OpenVolume(volume)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    const char filePath[] = "TESTDIR/TEST.TXT";

    if (!EnsureDirectory(volume, _testDir) || !EnsureFile(volume, filePath)) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "normalized path prep failed");

      return false;
    }

    const char expected[] = "Quantum";

    if (
      !WriteFileContents(
        volume,
        filePath,
        reinterpret_cast<const UInt8*>(expected),
        static_cast<UInt32>(sizeof(expected) - 1)
      )
    ) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "normalized write failed");

      return false;
    }

    FileSystem::Handle handle = FileSystem::Open(
      volume,
      "TESTDIR/../TESTDIR/TEST.TXT",
      0
    );

    if (handle == 0) {
      FileSystem::CloseVolume(volume);

      TEST_ASSERT(false, "normalized open failed");

      return false;
    }

    UInt8 buffer[16] = {};
    UInt32 bytes = FileSystem::Read(handle, buffer, sizeof(buffer) - 1);

    FileSystem::Close(handle);
    FileSystem::CloseVolume(volume);

    buffer[bytes] = '\0';

    bool match = true;

    for (UInt32 i = 0; expected[i] != '\0'; ++i) {
      if (buffer[i] != static_cast<UInt8>(expected[i])) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "normalized read");

    return match;
  }

  bool FAT12Tests::TestCleanup() {
    CleanupTestEntries();

    return true;
  }

  void FAT12Tests::RegisterTests() {
    Testing::Register("FAT12 load volume", TestVolumeLoad);
    Testing::Register("FAT12 volume info", TestVolumeInfo);
    Testing::Register("FAT12 root directory", TestRootDirectory);
    Testing::Register("FAT12 TESTDIR", TestSubDirectory);
    Testing::Register("FAT12 LFN root", TestLFNRoot);
    Testing::Register("FAT12 LFN file", TestLFNFile);
    Testing::Register("FAT12 TEST.TXT read", TestFileRead);
    Testing::Register("FAT12 TEST.TXT seek", TestFileSeek);
    Testing::Register("FAT12 append write", TestFileWriteAppend);
    Testing::Register("FAT12 create directory", TestCreateDirectory);
    Testing::Register("FAT12 create file", TestCreateFile);
    Testing::Register("FAT12 stat", TestStat);
    Testing::Register("FAT12 rename", TestRename);
    Testing::Register("FAT12 remove", TestRemove);
    Testing::Register("FAT12 path normalization", TestPathNormalization);
    Testing::Register("FAT12 cleanup", TestCleanup);
  }
}
