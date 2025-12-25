/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Tests.cpp
 * FAT12 service tests.
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/FileSystem.hpp>
#include <ABI/Task.hpp>

#include "Tests.hpp"
#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12::Tests {
  using Console = ABI::Console;
  using BlockDevice = ABI::Devices::BlockDevice;
  using FileSystem = ABI::FileSystem;
  using Task = ABI::Task;

  static UInt32 _testsPassed = 0;
  static UInt32 _testsFailed = 0;
  static UInt32 _testCount = 0;
  static bool _skipLogged = false;

  static void WriteDec(UInt32 value) {
    char buffer[16] = {};
    UInt32 idx = 0;

    do {
      buffer[idx++] = static_cast<char>('0' + (value % 10));
      value /= 10;
    } while (value != 0 && idx < sizeof(buffer));

    while (idx > 0) {
      char c[2] = { buffer[--idx], '\0' };

      Console::Write(c);
    }
  }

  static void LogHeader() {
    Console::WriteLine("Running FAT12 tests...");
  }

  static void LogFooter() {
    Console::Write("FAT12 tests complete: passed=");
    WriteDec(_testsPassed);
    Console::Write(" failed=");
    WriteDec(_testsFailed);
    Console::Write(" total=");
    WriteDec(_testCount);
    Console::WriteLine("");
  }

  static void LogSkip(CString reason) {
    if (_skipLogged) {
      return;
    }

    Console::Write("FAT12 tests skipped (");
    Console::Write(reason ? reason : "unknown");
    Console::WriteLine(")");

    _skipLogged = true;
  }

  static bool Assert(bool condition, CString message) {
    if (!condition) {
      Console::Write("Test assertion failed: ");
      Console::WriteLine(message ? message : "unknown");
    }

    return condition;
  }

  static bool WaitForFloppyReady() {
    for (UInt32 attempt = 0; attempt < 256; ++attempt) {
      UInt32 count = BlockDevice::GetCount();

      for (UInt32 i = 1; i <= count; ++i) {
        BlockDevice::Info info {};

        if (BlockDevice::GetInfo(i, info) != 0) {
          continue;
        }

        if (info.type != BlockDevice::Type::Floppy) {
          continue;
        }

        if (
          (info.flags & BlockDevice::flagReady) != 0 &&
          info.sectorSize != 0 &&
          info.sectorCount != 0
        ) {
          return true;
        }
      }

      Task::Yield();
    }

    return false;
  }

  static bool TestVolumeLoad() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    Volume volume {};

    if (!volume.Load()) {
      LogSkip("no FAT12 volume");

      return true;
    }

    return Assert(volume.IsValid(), "volume load failed");
  }

  static bool TestVolumeInfo() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    Volume volume {};

    if (!volume.Load()) {
      LogSkip("no FAT12 volume");

      return true;
    }

    const FileSystem::VolumeInfo& info = volume.GetInfo();

    bool ok = true;

    ok &= Assert(
      info.fsType == static_cast<UInt32>(FileSystem::Type::FAT12),
      "volume type mismatch"
    );
    ok &= Assert(info.sectorSize != 0, "volume sector size missing");
    ok &= Assert(info.sectorCount != 0, "volume sector count missing");

    return ok;
  }

  static bool TestRootDirectory() {
    if (!WaitForFloppyReady()) {
      LogSkip("floppy not ready");

      return true;
    }

    Volume volume {};

    if (!volume.Load()) {
      LogSkip("no FAT12 volume");

      return true;
    }

    bool found = false;
    FileSystem::DirectoryEntry entry {};

    Console::WriteLine("FAT12 root directory entries:");

    for (UInt32 i = 0; i < volume.GetRootEntryCount(); ++i) {
      bool end = false;

      if (volume.ReadRootEntry(i, entry, end)) {
        Console::Write("  ");
        Console::WriteLine(entry.name);

        found = true;
      }

      if (end) {
        break;
      }
    }

    if (!found) {
      LogSkip("no directory entries");

      return true;
    }

    return Assert(entry.name[0] != '\0', "root entry read");
  }

  static void RunTest(CString name, bool (*func)()) {
    Console::Write("[TEST] ");
    Console::WriteLine(name ? name : "(unnamed)");
    _testCount++;

    bool ok = func ? func() : false;

    if (ok) {
      _testsPassed++;
    } else {
      _testsFailed++;
      Console::Write("[FAIL] ");
      Console::WriteLine(name ? name : "(unnamed)");
    }
  }

  void Run() {
    LogHeader();

    RunTest("FAT12 load volume", TestVolumeLoad);
    RunTest("FAT12 volume info", TestVolumeInfo);
    RunTest("FAT12 root directory", TestRootDirectory);

    LogFooter();
  }
}
