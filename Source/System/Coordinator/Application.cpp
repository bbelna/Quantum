/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Application.cpp
 * System coordinator implementation.
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/IO.hpp>
#include <ABI/Prelude.hpp>
#include <ABI/Task.hpp>

#include "Application.hpp"
#include "Testing.hpp"

namespace Quantum::System::Coordinator {
  using Console = ABI::Console;
  using BlockDevice = ABI::Devices::BlockDevice;
  using IO = ABI::IO;
  using Task = ABI::Task;

  bool Application::HasMagic(const BundleHeader& header) {
    const char expected[8] = { 'I','N','I','T','B','N','D','\0' };

    for (UInt32 i = 0; i < 8; ++i) {
      if (header.magic[i] != expected[i]) {
        return false;
      }
    }

    return true;
  }

  UInt32 Application::EntryNameLength(const BundleEntry& entry) {
    UInt32 len = 0;

    while (len < 32 && entry.name[len] != '\0') {
      ++len;
    }

    return len;
  }

  bool Application::EntryNameEquals(const BundleEntry& entry, CString name) {
    if (!name) {
      return false;
    }

    for (UInt32 i = 0; i < 32; ++i) {
      char entryChar = entry.name[i];
      char nameChar = name[i];

      if (entryChar != nameChar) {
        return false;
      }

      if (entryChar == '\0') {
        return true;
      }
    }

    return false;
  }

  void Application::SpawnEntry(const BundleEntry& entry) {
    if (entry.type == 1) {
      return;
    }

    UInt32 taskId = InitBundle::Spawn(entry.name);

    if (taskId == 0) {
      Console::WriteLine("Failed to spawn INIT.BND entry");

      return;
    }

    if (EntryNameEquals(entry, "floppy")) {
      if (!_floppyPresent) {
        Console::WriteLine("Floppy device not detected; skipping driver");

        return;
      }

      UInt32 grant = IO::GrantIOAccess(taskId);

      if (grant == 0) {
        Console::WriteLine("Floppy driver granted I/O access");
      } else {
        Console::WriteLine("Failed to grant I/O access to floppy driver");
      }
    }
  }

  bool Application::HasFloppyDevice() {
    UInt32 count = BlockDevice::GetCount();
    bool found = false;

    _floppyDeviceId = 0;

    for (UInt32 i = 1; i <= count; ++i) {
      BlockDevice::Info info {};

      if (BlockDevice::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type == BlockDevice::Type::Floppy) {
        found = true;
        _floppyDeviceId = info.id;

        break;
      }
    }

    if (found) {
      Console::WriteLine("Floppy device detected");
    } else {
      Console::WriteLine("Floppy device not detected");
    }

    return found;
  }

  void Application::Main() {
    Console::WriteLine("Coordinator initialized");

    _floppyPresent = HasFloppyDevice();

    InitBundle::Info info {};

    bool ok = InitBundle::GetInfo(info);

    if (!ok || info.base == 0 || info.size == 0) {
      Console::WriteLine("INIT.BND not available");
      Task::Exit(1);
    }

    const UInt8* base = reinterpret_cast<const UInt8*>(info.base);

    if (info.size < sizeof(BundleHeader)) {
      Console::WriteLine("INIT.BND too small");
      Task::Exit(1);
    }

    const BundleHeader* header = reinterpret_cast<const BundleHeader*>(base);

    if (!HasMagic(*header)) {
      Console::WriteLine("INIT.BND bad magic");
      Task::Exit(1);
    }

    UInt32 tableOffset = header->tableOffset;
    UInt32 entryCount = header->entryCount;
    UInt32 tableBytes = entryCount * static_cast<UInt32>(sizeof(BundleEntry));

    if (tableOffset + tableBytes > info.size) {
      Console::WriteLine("INIT.BND table out of range");
      Task::Exit(1);
    }

    Console::WriteLine("INIT entries:");

    const BundleEntry* entries
      = reinterpret_cast<const BundleEntry*>(base + tableOffset);

    for (UInt32 i = 0; i < entryCount; ++i) {
      const BundleEntry& entry = entries[i];
      UInt32 nameLen = EntryNameLength(entry);

      if (nameLen > 0) {
        char line[40] = {};
        UInt32 writeIndex = 0;

        line[writeIndex++] = ' ';
        line[writeIndex++] = ' ';

        for (UInt32 j = 0; j < 32 && entry.name[j] != '\0'; ++j) {
          if (writeIndex + 1 >= sizeof(line)) {
            break;
          }

          line[writeIndex++] = entry.name[j];
        }

        line[writeIndex] = '\0';

        Console::WriteLine(line);
      } else {
        Console::WriteLine("  (unnamed)");
      }

      SpawnEntry(entry);
    }

    Testing::RegisterBuiltins();
    Testing::RunAll();

    Task::Exit(0);
  }
}
