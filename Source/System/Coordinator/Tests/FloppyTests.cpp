/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Tests/FloppyTests.cpp
 * Coordinator floppy test suite.
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/Task.hpp>

#include "Macros.hpp"
#include "Tests/FloppyTests.hpp"

namespace Quantum::System::Coordinator::Tests {
  using Console = ABI::Console;
  using BlockDevice = ABI::Devices::BlockDevice;
  using Task = ABI::Task;

  static bool _skipLogged = false;

  static void LogSkip(CString reason) {
    if (_skipLogged) {
      return;
    }

    Console::Write("Floppy tests skipped (");
    Console::Write(reason ? reason : "unknown");
    Console::WriteLine(")");

    _skipLogged = true;
  }

  static bool FindFloppyDevice(UInt32& deviceId, BlockDevice::Info& info) {
    UInt32 count = BlockDevice::GetCount();

    deviceId = 0;

    for (UInt32 i = 1; i <= count; ++i) {
      BlockDevice::Info candidate {};

      if (BlockDevice::GetInfo(i, candidate) != 0) {
        continue;
      }

      if (candidate.type == BlockDevice::Type::Floppy) {
        deviceId = candidate.id;
        info = candidate;

        return true;
      }
    }

    return false;
  }

  static bool WaitForReady(UInt32 deviceId, BlockDevice::Info& info) {
    for (UInt32 i = 0; i < 64; ++i) {
      if (BlockDevice::GetInfo(deviceId, info) == 0) {
        // Require geometry so reads don't race driver init.
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

  static bool PrepareFloppy(
    UInt32& deviceId,
    BlockDevice::Info& info,
    bool requireWrite
  ) {
    if (!FindFloppyDevice(deviceId, info)) {
      LogSkip("no device");

      return false;
    }

    if (!WaitForReady(deviceId, info)) {
      LogSkip("not ready");

      return false;
    }

    if (requireWrite && (info.flags & BlockDevice::flagReadOnly) != 0) {
      LogSkip("read-only");

      return false;
    }

    return true;
  }

  static bool TestSingleSectorRead() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};

    if (!PrepareFloppy(deviceId, info, false)) {
      return true;
    }

    UInt8 buffer[512] = {};
    BlockDevice::Request request {};

    request.deviceId = deviceId;
    request.lba = 0;
    request.count = 1;
    request.buffer = buffer;

    UInt32 result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy read failed");

    if (result != 0) {
      return false;
    }

    UInt32 nonZeroCount = 0;

    for (UInt32 i = 0; i < sizeof(buffer); ++i) {
      if (buffer[i] != 0) {
        nonZeroCount++;
      }
    }

    TEST_ASSERT(nonZeroCount > 0, "Floppy read empty data");

    return nonZeroCount > 0;
  }

  static bool TestMultiSectorRead() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};

    if (!PrepareFloppy(deviceId, info, false)) {
      return true;
    }

    UInt8 buffer[1024] = {};
    BlockDevice::Request request {};

    request.deviceId = deviceId;
    request.lba = 0;
    request.count = 2;
    request.buffer = buffer;

    UInt32 result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy multi-sector read failed");

    if (result != 0) {
      return false;
    }

    UInt32 nonZeroCount = 0;

    for (UInt32 i = 0; i < sizeof(buffer); ++i) {
      if (buffer[i] != 0) {
        nonZeroCount++;
      }
    }

    TEST_ASSERT(nonZeroCount > 0, "Floppy multi-sector read empty data");

    return nonZeroCount > 0;
  }

  static bool TestWriteReadback() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};

    if (!PrepareFloppy(deviceId, info, true)) {
      return true;
    }

    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;

    if (sectorSize > 512 || sectorSize == 0) {
      LogSkip("sector size");

      return true;
    }

    UInt32 totalSectors = info.sectorCount;

    if (totalSectors == 0) {
      LogSkip("sector count");

      return true;
    }

    UInt32 scratchLba = totalSectors > 1 ? totalSectors - 1 : 0;
    UInt8 original[512] = {};
    UInt8 writeData[512] = {};
    UInt8 verify[512] = {};
    BlockDevice::Request request {};

    request.deviceId = deviceId;
    request.count = 1;
    request.lba = scratchLba;
    request.buffer = original;

    UInt32 result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy write test read original failed");

    if (result != 0) {
      return false;
    }

    for (UInt32 i = 0; i < sectorSize; ++i) {
      writeData[i] = static_cast<UInt8>(0xA5 ^ i);
    }

    request.buffer = writeData;
    result = BlockDevice::Write(request);

    TEST_ASSERT(result == 0, "Floppy write failed");

    if (result != 0) {
      return false;
    }

    request.buffer = verify;
    result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy write verify read failed");

    if (result != 0) {
      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < sectorSize; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "Floppy write verify mismatch");

    request.buffer = original;

    BlockDevice::Write(request);

    return match;
  }

  static bool TestMultiSectorWriteReadback() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};

    if (!PrepareFloppy(deviceId, info, true)) {
      return true;
    }

    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;
    constexpr UInt32 sectorCount = 2;
    constexpr UInt32 maxBytes = 2048;
    UInt32 totalBytes = sectorSize * sectorCount;

    if (sectorSize == 0 || totalBytes > maxBytes) {
      LogSkip("sector size");

      return true;
    }

    UInt32 totalSectors = info.sectorCount;

    if (totalSectors < sectorCount) {
      LogSkip("sector count");

      return true;
    }

    UInt32 scratchLba = totalSectors - sectorCount;
    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};
    BlockDevice::Request request {};

    request.deviceId = deviceId;
    request.count = sectorCount;
    request.lba = scratchLba;
    request.buffer = original;

    UInt32 result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy multi-sector read original failed");

    if (result != 0) {
      return false;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x5A ^ i);
    }

    request.buffer = writeData;
    result = BlockDevice::Write(request);

    TEST_ASSERT(result == 0, "Floppy multi-sector write failed");

    if (result != 0) {
      return false;
    }

    request.buffer = verify;
    result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy multi-sector verify read failed");

    if (result != 0) {
      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "Floppy multi-sector verify mismatch");

    request.buffer = original;

    BlockDevice::Write(request);

    return match;
  }

  static bool TestCrossTrackWriteReadback() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};

    if (!PrepareFloppy(deviceId, info, true)) {
      return true;
    }

    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;
    constexpr UInt32 sectorCount = 4;
    constexpr UInt32 maxBytes = 2048;
    constexpr UInt32 assumedSectorsPerTrack = 18;
    UInt32 totalBytes = sectorSize * sectorCount;

    if (sectorSize == 0 || totalBytes > maxBytes) {
      LogSkip("sector size");

      return true;
    }

    UInt32 totalSectors = info.sectorCount;

    if (totalSectors < sectorCount) {
      LogSkip("sector count");

      return true;
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
    BlockDevice::Request request {};

    request.deviceId = deviceId;
    request.count = sectorCount;
    request.lba = desiredLba;
    request.buffer = original;

    UInt32 result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy cross-track read original failed");

    if (result != 0) {
      return false;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x3C ^ i);
    }

    request.buffer = writeData;
    result = BlockDevice::Write(request);

    TEST_ASSERT(result == 0, "Floppy cross-track write failed");

    if (result != 0) {
      return false;
    }

    request.buffer = verify;
    result = BlockDevice::Read(request);

    TEST_ASSERT(result == 0, "Floppy cross-track verify read failed");

    if (result != 0) {
      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "Floppy cross-track verify mismatch");

    request.buffer = original;

    BlockDevice::Write(request);

    return match;
  }

  void FloppyTests::RegisterTests() {
    Testing::Register("Floppy single-sector read", TestSingleSectorRead);
    Testing::Register("Floppy multi-sector read", TestMultiSectorRead);
    Testing::Register("Floppy write/readback", TestWriteReadback);
    Testing::Register(
      "Floppy multi-sector write/readback",
      TestMultiSectorWriteReadback
    );
    Testing::Register(
      "Floppy cross-track write/readback",
      TestCrossTrackWriteReadback
    );
  }
}
