/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Tests/FAT12Tests.cpp
 * Coordinator FAT12 test suite.
 */

#include <ABI/Console.hpp>
#include <ABI/FileSystem.hpp>

#include "Macros.hpp"
#include "Tests/FAT12Tests.hpp"

namespace Quantum::System::Coordinator::Tests {
  using Console = ABI::Console;
  using FileSystem = ABI::FileSystem;

  static bool _skipLogged = false;

  static void LogSkip(CString reason) {
    if (_skipLogged) {
      return;
    }

    Console::Write("FAT12 tests skipped (");
    Console::Write(reason ? reason : "unknown");
    Console::WriteLine(")");

    _skipLogged = true;
  }

  static bool FindFAT12Volume(FileSystem::VolumeEntry& entryOut) {
    FileSystem::VolumeEntry entries[4] = {};
    UInt32 count = FileSystem::ListVolumes(entries, 4);

    if (count == 0) {
      return false;
    }

    for (UInt32 i = 0; i < count; ++i) {
      if (entries[i].fsType == static_cast<UInt32>(FileSystem::Type::FAT12)) {
        entryOut = entries[i];

        return true;
      }
    }

    return false;
  }

  static bool TestListVolumes() {
    FileSystem::VolumeEntry entry {};

    if (!FindFAT12Volume(entry)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    TEST_ASSERT(entry.label[0] != '\0', "FAT12 label empty");

    return entry.label[0] != '\0';
  }

  static bool TestGetVolumeInfo() {
    FileSystem::VolumeEntry entry {};

    if (!FindFAT12Volume(entry)) {
      LogSkip("no FAT12 volume");

      return true;
    }

    // use label to open the volume so the service owns the mapping
    FileSystem::VolumeHandle handle = FileSystem::OpenVolume(entry.label);

    TEST_ASSERT(handle != 0, "OpenVolume failed");

    if (handle == 0) {
      return false;
    }

    FileSystem::VolumeInfo info {};
    UInt32 result = FileSystem::GetVolumeInfo(handle, info);

    FileSystem::CloseVolume(handle);

    TEST_ASSERT(result == 0, "GetVolumeInfo failed");

    if (result != 0) {
      return false;
    }

    TEST_ASSERT(
      info.fsType == static_cast<UInt32>(FileSystem::Type::FAT12),
      "FAT12 volume type mismatch"
    );
    TEST_ASSERT(info.sectorSize != 0, "Volume sector size missing");
    TEST_ASSERT(info.sectorCount != 0, "Volume sector count missing");

    return info.sectorSize != 0 && info.sectorCount != 0;
  }

  void FAT12Tests::RegisterTests() {
    Testing::Register("FAT12 list volumes", TestListVolumes);
    Testing::Register("FAT12 volume info", TestGetVolumeInfo);
  }
}
