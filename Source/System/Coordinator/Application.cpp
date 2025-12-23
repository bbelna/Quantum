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

  void Application::TestFloppyBlockRead() {
    if (_floppyDeviceId == 0) {
      return;
    }

    BlockDevice::Info info {};
    bool ready = false;

    for (UInt32 i = 0; i < 64; ++i) {
      if (BlockDevice::GetInfo(_floppyDeviceId, info) == 0) {
        if ((info.flags & BlockDevice::flagReady) != 0) {
          ready = true;

          break;
        }
      }

      Task::Yield();
    }

    if (!ready) {
      Console::WriteLine("Floppy block device not ready; skipping test");

      return;
    }

    UInt8 buffer[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _floppyDeviceId;
    request.lba = 0;
    request.count = 1;
    request.buffer = buffer;

    UInt32 result = BlockDevice::Read(request);

    if (result == 0) {
      UInt32 nonZeroCount = 0;

      for (UInt32 i = 0; i < sizeof(buffer); ++i) {
        if (buffer[i] != 0) {
          nonZeroCount++;
        }
      }

      if (nonZeroCount == 0) {
        Console::WriteLine("Floppy block read test returned empty data");
      } else {
        Console::WriteLine("Floppy block read test succeeded");
      }
    } else {
      Console::WriteLine("Floppy block read test returned error");
    }
  }

  void Application::TestFloppyMultiSectorRead() {
    if (_floppyDeviceId == 0) {
      return;
    }

    UInt8 buffer[1024] = {};
    BlockDevice::Request request {};

    request.deviceId = _floppyDeviceId;
    request.lba = 0;
    request.count = 2;
    request.buffer = buffer;

    UInt32 result = BlockDevice::Read(request);

    if (result == 0) {
      UInt32 nonZeroCount = 0;

      for (UInt32 i = 0; i < sizeof(buffer); ++i) {
        if (buffer[i] != 0) {
          nonZeroCount++;
        }
      }

      if (nonZeroCount == 0) {
        Console::WriteLine("Floppy multi-sector read returned empty data");
      } else {
        Console::WriteLine("Floppy multi-sector read succeeded");
      }
    } else {
      Console::WriteLine("Floppy multi-sector read returned error");
    }
  }

  void Application::TestFloppyWriteReadback() {
    if (_floppyDeviceId == 0) {
      return;
    }

    BlockDevice::Info info {};

    if (BlockDevice::GetInfo(_floppyDeviceId, info) != 0) {
      return;
    }

    if ((info.flags & BlockDevice::flagReadOnly) != 0) {
      Console::WriteLine("Floppy write/read test skipped (read-only)");

      return;
    }

    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;

    if (sectorSize > 512) {
      Console::WriteLine("Floppy write/read test skipped (sector size)");

      return;
    }

    UInt32 totalSectors = info.sectorCount;

    if (totalSectors == 0) {
      Console::WriteLine("Floppy write/read test skipped (sector count)");

      return;
    }

    UInt32 scratchLba = totalSectors > 1 ? totalSectors - 1 : 0;

    UInt8 original[512] = {};
    UInt8 writeData[512] = {};
    UInt8 verify[512] = {};
    BlockDevice::Request request {};

    request.deviceId = _floppyDeviceId;
    request.count = 1;
    request.lba = scratchLba;
    request.buffer = original;

    if (BlockDevice::Read(request) != 0) {
      Console::WriteLine("Floppy write/read test failed (read original)");

      return;
    }

    for (UInt32 i = 0; i < sectorSize; ++i) {
      writeData[i] = static_cast<UInt8>(0xA5 ^ i);
    }

    request.buffer = writeData;

    if (BlockDevice::Write(request) != 0) {
      Console::WriteLine("Floppy write/read test failed (write)");

      return;
    }

    request.buffer = verify;

    if (BlockDevice::Read(request) != 0) {
      Console::WriteLine("Floppy write/read test failed (read verify)");

      return;
    }

    bool match = true;

    for (UInt32 i = 0; i < sectorSize; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    if (match) {
      Console::WriteLine("Floppy write/read test succeeded");
    } else {
      Console::WriteLine("Floppy write/read test mismatch");
    }

    request.buffer = original;

    if (BlockDevice::Write(request) != 0) {
      Console::WriteLine("Floppy write/read test failed (restore)");
    }
  }

  void Application::TestFloppyMultiSectorWriteReadback() {
    if (_floppyDeviceId == 0) {
      return;
    }

    BlockDevice::Info info {};

    if (BlockDevice::GetInfo(_floppyDeviceId, info) != 0) {
      return;
    }

    if ((info.flags & BlockDevice::flagReadOnly) != 0) {
      Console::WriteLine("Floppy multi-sector write skipped (read-only)");

      return;
    }

    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;
    constexpr UInt32 sectorCount = 2;
    constexpr UInt32 maxBytes = 2048;
    UInt32 totalBytes = sectorSize * sectorCount;

    if (sectorSize == 0 || totalBytes > maxBytes) {
      Console::WriteLine("Floppy multi-sector write skipped (sector size)");

      return;
    }

    UInt32 totalSectors = info.sectorCount;

    if (totalSectors < sectorCount) {
      Console::WriteLine("Floppy multi-sector write skipped (sector count)");

      return;
    }

    UInt32 scratchLba = totalSectors - sectorCount;
    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};
    BlockDevice::Request request {};

    request.deviceId = _floppyDeviceId;
    request.count = sectorCount;
    request.lba = scratchLba;
    request.buffer = original;

    if (BlockDevice::Read(request) != 0) {
      Console::WriteLine("Floppy multi-sector write failed (read original)");

      return;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x5A ^ i);
    }

    request.buffer = writeData;

    if (BlockDevice::Write(request) != 0) {
      Console::WriteLine("Floppy multi-sector write failed (write)");

      return;
    }

    request.buffer = verify;

    if (BlockDevice::Read(request) != 0) {
      Console::WriteLine("Floppy multi-sector write failed (read verify)");

      return;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    if (match) {
      Console::WriteLine("Floppy multi-sector write succeeded");
    } else {
      Console::WriteLine("Floppy multi-sector write mismatch");
    }

    request.buffer = original;

    if (BlockDevice::Write(request) != 0) {
      Console::WriteLine("Floppy multi-sector write failed (restore)");
    }
  }

  void Application::TestFloppyCrossTrackWriteReadback() {
    if (_floppyDeviceId == 0) {
      return;
    }

    BlockDevice::Info info{};

    if (BlockDevice::GetInfo(_floppyDeviceId, info) != 0) {
      return;
    }

    if ((info.flags & BlockDevice::flagReadOnly) != 0) {
      Console::WriteLine("Floppy cross-track write skipped (read-only)");

      return;
    }

    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;
    constexpr UInt32 sectorCount = 4;
    constexpr UInt32 maxBytes = 2048;
    constexpr UInt32 assumedSectorsPerTrack = 18;
    UInt32 totalBytes = sectorSize * sectorCount;

    if (sectorSize == 0 || totalBytes > maxBytes) {
      Console::WriteLine("Floppy cross-track write skipped (sector size)");

      return;
    }

    UInt32 totalSectors = info.sectorCount;

    if (totalSectors < sectorCount) {
      Console::WriteLine("Floppy cross-track write skipped (sector count)");

      return;
    }

    UInt32 scratchLba = totalSectors - sectorCount;
    UInt32 trackBase = (scratchLba / assumedSectorsPerTrack)
      * assumedSectorsPerTrack;
    UInt32 desiredLba = trackBase + (assumedSectorsPerTrack - 2);

    if (desiredLba + sectorCount > totalSectors) {
      desiredLba = scratchLba;
    }

    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};

    BlockDevice::Request request{};
    request.deviceId = _floppyDeviceId;
    request.count = sectorCount;
    request.lba = desiredLba;
    request.buffer = original;

    if (BlockDevice::Read(request) != 0) {
      Console::WriteLine("Floppy cross-track write failed (read original)");

      return;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x3C ^ i);
    }

    request.buffer = writeData;

    if (BlockDevice::Write(request) != 0) {
      Console::WriteLine("Floppy cross-track write failed (write)");

      return;
    }

    request.buffer = verify;

    if (BlockDevice::Read(request) != 0) {
      Console::WriteLine("Floppy cross-track write failed (read verify)");

      return;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    if (match) {
      Console::WriteLine("Floppy cross-track write succeeded");
    } else {
      Console::WriteLine("Floppy cross-track write mismatch");
    }

    request.buffer = original;

    if (BlockDevice::Write(request) != 0) {
      Console::WriteLine("Floppy cross-track write failed (restore)");
    }
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

    if (_floppyPresent) {
      TestFloppyBlockRead();
      TestFloppyMultiSectorRead();
      TestFloppyWriteReadback();
      TestFloppyMultiSectorWriteReadback();
      TestFloppyCrossTrackWriteReadback();
    }

    Console::WriteLine("INIT.BND parsed");
    Task::Exit(0);
  }
}
