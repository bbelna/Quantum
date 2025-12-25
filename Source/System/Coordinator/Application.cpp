/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Application.cpp
 * System coordinator implementation.
 */

#include <ABI/Console.hpp>
#include <ABI/Coordinator.hpp>
#include <ABI/InitBundle.hpp>
#include <ABI/IO.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Prelude.hpp>
#include <ABI/Task.hpp>

#include "Application.hpp"
#include "FileSystem.hpp"
#include "IRQ.hpp"

namespace Quantum::System::Coordinator {
  using Console = ABI::Console;
  using IO = ABI::IO;
  using IPC = ABI::IPC;
  using Task = ABI::Task;
  using FileSystem = Coordinator::FileSystem;
  using IRQ = Coordinator::IRQ;

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

  bool Application::SpawnEntry(const BundleEntry& entry) {
    // don't respawn coordinator
    if (entry.type == InitBundle::EntryType::Init) {
      return false;
    }

    UInt32 taskId = InitBundle::Spawn(entry.name);

    if (taskId == 0) {
      Console::WriteLine("Failed to spawn INIT.BND entry");

      return false;
    }

    // grant I/O access to drivers
    if (entry.type == InitBundle::EntryType::Driver) {
      UInt32 grant = IO::GrantIOAccess(taskId);

      if (grant == 0) {
        Console::Write("Granted I/O access to ");
        Console::WriteLine(entry.name);
      } else {
        Console::Write("Failed to grant I/O access to ");
        Console::WriteLine(entry.name);
      }
    }

    return true;
  }

  UInt8 Application::DeviceMaskFromId(UInt8 deviceId) {
    if (deviceId == 0 || deviceId > 8) {
      return 0;
    }

    return static_cast<UInt8>(1u << (deviceId - 1));
  }

  UInt8 Application::ReadCMOS(UInt8 reg) {
    IO::Out8(0x70, static_cast<UInt8>(reg | 0x80));

    return IO::In8(0x71);
  }

  bool Application::HasFloppyDevice() {
    UInt8 types = ReadCMOS(0x10);
    UInt8 typeA = static_cast<UInt8>((types >> 4) & 0x0F);
    UInt8 typeB = static_cast<UInt8>(types & 0x0F);

    if (types == 0) {
      return true;
    }

    return typeA != 0 || typeB != 0;
  }

  UInt8 Application::DetectDevices() {
    UInt8 detected = 0;

    if (HasFloppyDevice()) {
      detected |= DeviceMaskFromId(
        static_cast<UInt8>(DeviceType::Floppy)
      );
    }

    return detected;
  }

  void Application::ProcessReadyMessages() {
    if (_readyPortId == 0) {
      return;
    }

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(_readyPortId, msg) != 0) {
        break;
      }

      if (msg.length < sizeof(ABI::Coordinator::ReadyMessage)) {
        continue;
      }

      ABI::Coordinator::ReadyMessage ready {};

      for (UInt32 i = 0; i < sizeof(ready); ++i) {
        reinterpret_cast<UInt8*>(&ready)[i] = msg.payload[i];
      }

      if (ready.state == 0) {
        continue;
      }

      UInt8 mask = DeviceMaskFromId(static_cast<UInt8>(ready.deviceId));

      if (mask != 0) {
        _readyDevices |= mask;
      }
    }
  }

  void Application::Main() {
    #if defined(DEBUG)
    Console::WriteLine("Coordinator initialized");
    #endif

    IRQ::Initialize();
    FileSystem::Initialize();

    _readyPortId = IPC::CreatePort();

    if (_readyPortId == 0) {
      Console::WriteLine("Coordinator: failed to create readiness port");
    } else if (_readyPortId != IPC::Ports::CoordinatorReady) {
      Console::WriteLine("Coordinator: readiness port id mismatch");
    }

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

    #if defined(DEBUG)
    Console::WriteLine("INIT entries:");
    #endif

    const BundleEntry* entries
      = reinterpret_cast<const BundleEntry*>(base + tableOffset);

    if (entryCount > _maxBundleEntries) {
      Console::WriteLine("INIT.BND entry count capped");

      entryCount = _maxBundleEntries;
    }

    _detectedDevices = DetectDevices();
    _spawnedDevices = 0;
    _readyDevices = 0;

    bool spawned[_maxBundleEntries] = {};

    for (UInt32 i = 0; i < entryCount; ++i) {
      const BundleEntry& entry = entries[i];

      #if defined(DEBUG)
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
      #endif

      if (entry.type == InitBundle::EntryType::Init) {
        continue;
      }

      UInt8 deviceMask = DeviceMaskFromId(entry.device);

      if (deviceMask != 0 && (_detectedDevices & deviceMask) == 0) {
        if ((entry.flags & 0x01) != 0) {
          Console::WriteLine("Required device missing; entry skipped");
        }

        spawned[i] = true;

        continue;
      }

      if (entry.dependsMask != 0) {
        continue;
      }

      if (SpawnEntry(entry) && deviceMask != 0) {
        _spawnedDevices |= deviceMask;
      }

      spawned[i] = true;
    }

    for (;;) {
      ProcessReadyMessages();
      bool progressed = false;

      for (UInt32 i = 0; i < entryCount; ++i) {
        const BundleEntry& entry = entries[i];

        if (spawned[i]) {
          continue;
        }

        if (entry.type == InitBundle::EntryType::Init) {
          spawned[i] = true;

          continue;
        }

        if (entry.dependsMask == 0) {
          spawned[i] = true;

          continue;
        }

        UInt8 deviceMask = DeviceMaskFromId(entry.device);

        if (deviceMask != 0 && (_detectedDevices & deviceMask) == 0) {
          if ((entry.flags & 0x01) != 0) {
            Console::WriteLine("Required device missing; entry skipped");
          }

          spawned[i] = true;

          continue;
        }

        if ((entry.dependsMask & _readyDevices) != entry.dependsMask) {
          continue;
        }

        if (SpawnEntry(entry)) {
          if (deviceMask != 0) {
            _spawnedDevices |= deviceMask;
          }

          spawned[i] = true;
          progressed = true;
        }
      }

      if (!progressed) {
        Task::Yield();
      }

      IRQ::ProcessPending();
      FileSystem::ProcessPending();
    }
  }
}
