/**
 * @file Applications/Diagnostics/TestSuite/Tests/FloppyTests.cpp
 * @brief Floppy block device tests.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/Devices/DeviceBroker.hpp>
#include <ABI/Handle.hpp>
#include <ABI/Task.hpp>

#include "Testing.hpp"
#include "Tests/FloppyTests.hpp"

namespace Quantum::Applications::Diagnostics::TestSuite::Tests {
  using ABI::Console;
  using ABI::Devices::BlockDevices;
  using ABI::Task;

  void FloppyTests::LogSkip(CString reason) {
    if (_skipLogged) {
      return;
    }

    Console::Write("Floppy tests skipped (");
    Console::Write(reason ? reason : "unknown");
    Console::WriteLine(")");

    _skipLogged = true;
  }

  bool FloppyTests::FindFloppyDevice(
    UInt32& outToken,
    BlockDevices::Info& outInfo
  ) {
    UInt32 count = BlockDevices::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      BlockDevices::Info info {};

      if (BlockDevices::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type != BlockDevices::Type::Floppy) {
        continue;
      }

      if ((info.flags & static_cast<UInt32>(BlockDevices::Flag::Ready)) == 0) {
        continue;
      }

      UInt32 handle = ABI::Devices::DeviceBroker::OpenBlockDevice(
        info.id,
        static_cast<UInt32>(BlockDevices::Right::Read)
          | static_cast<UInt32>(BlockDevices::Right::Write)
          | static_cast<UInt32>(BlockDevices::Right::Control)
          | static_cast<UInt32>(BlockDevices::Right::Bind)
      );

      if (handle == 0) {
        handle = BlockDevices::Open(
          info.id,
          static_cast<UInt32>(BlockDevices::Right::Read)
            | static_cast<UInt32>(BlockDevices::Right::Write)
            | static_cast<UInt32>(BlockDevices::Right::Control)
            | static_cast<UInt32>(BlockDevices::Right::Bind)
        );
      }

      outToken = handle != 0 ? handle : info.id;
      outInfo = info;

      return true;
    }

    return false;
  }

  void FloppyTests::CloseDeviceToken(UInt32 deviceToken, UInt32 deviceId) {
    if (deviceToken != 0 && deviceToken != deviceId) {
      ABI::Handle::Close(deviceToken);
    }
  }

  bool FloppyTests::ReadSectors(
    UInt32 deviceToken,
    UInt32 lba,
    UInt32 count,
    void* buffer
  ) {
    BlockDevices::Request request {};

    request.deviceId = deviceToken;
    request.lba = lba;
    request.count = count;
    request.buffer = buffer;

    return BlockDevices::Read(request) == 0;
  }

  bool FloppyTests::WriteSectors(
    UInt32 deviceToken,
    UInt32 lba,
    UInt32 count,
    void* buffer
  ) {
    BlockDevices::Request request {};

    request.deviceId = deviceToken;
    request.lba = lba;
    request.count = count;
    request.buffer = buffer;

    return BlockDevices::Write(request) == 0;
  }

  bool FloppyTests::TestSingleSectorRead() {
    UInt32 deviceToken = 0;
    BlockDevices::Info info {};

    if (!FindFloppyDevice(deviceToken, info)) {
      LogSkip("no device");

      return true;
    }

    constexpr UInt32 maxBytes = 4096;

    if (info.sectorSize == 0 || info.sectorSize > maxBytes) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt8 buffer[maxBytes] = {};

    if (!ReadSectors(deviceToken, 0, 1, buffer)) {
      TEST_ASSERT(false, "floppy read failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    UInt32 nonZeroCount = 0;

    for (UInt32 i = 0; i < info.sectorSize; ++i) {
      if (buffer[i] != 0) {
        nonZeroCount++;
      }
    }

    if (nonZeroCount == 0) {
      TEST_ASSERT(false, "floppy read empty data");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    CloseDeviceToken(deviceToken, info.id);

    return true;
  }

  bool FloppyTests::TestMultiSectorRead() {
    UInt32 deviceToken = 0;
    BlockDevices::Info info {};

    if (!FindFloppyDevice(deviceToken, info)) {
      LogSkip("no device");

      return true;
    }

    constexpr UInt32 maxBytes = 4096;
    constexpr UInt32 count = 2;

    if (info.sectorSize == 0) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt32 totalBytes = info.sectorSize * count;

    if (totalBytes > maxBytes) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt8 buffer[maxBytes] = {};

    if (!ReadSectors(deviceToken, 0, count, buffer)) {
      TEST_ASSERT(false, "floppy multi-sector read failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    UInt32 nonZeroCount = 0;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (buffer[i] != 0) {
        nonZeroCount++;
      }
    }

    if (nonZeroCount == 0) {
      TEST_ASSERT(false, "floppy multi-sector read empty data");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    CloseDeviceToken(deviceToken, info.id);

    return true;
  }

  bool FloppyTests::TestWriteReadback() {
    UInt32 deviceToken = 0;
    BlockDevices::Info info {};

    if (!FindFloppyDevice(deviceToken, info)) {
      LogSkip("no device");

      return true;
    }

    if ((info.flags & static_cast<UInt32>(BlockDevices::Flag::ReadOnly)) != 0) {
      LogSkip("read-only");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    constexpr UInt32 maxBytes = 4096;

    if (info.sectorSize == 0 || info.sectorSize > maxBytes) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    if (info.sectorCount == 0) {
      LogSkip("sector count");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt32 scratchLBA = info.sectorCount > 1
      ? info.sectorCount - 1
      : 0;
    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};

    if (!ReadSectors(deviceToken, scratchLBA, 1, original)) {
      TEST_ASSERT(false, "floppy write test read original failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    for (UInt32 i = 0; i < info.sectorSize; ++i) {
      writeData[i] = static_cast<UInt8>(0xA5 ^ i);
    }

    if (!WriteSectors(deviceToken, scratchLBA, 1, writeData)) {
      TEST_ASSERT(false, "floppy write failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    if (!ReadSectors(deviceToken, scratchLBA, 1, verify)) {
      TEST_ASSERT(false, "floppy write verify read failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < info.sectorSize; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "floppy write verify mismatch");

    WriteSectors(deviceToken, scratchLBA, 1, original);

    CloseDeviceToken(deviceToken, info.id);

    return match;
  }

  bool FloppyTests::TestMultiSectorWriteReadback() {
    UInt32 deviceToken = 0;
    BlockDevices::Info info {};

    if (!FindFloppyDevice(deviceToken, info)) {
      LogSkip("no device");

      return true;
    }

    if ((info.flags & static_cast<UInt32>(BlockDevices::Flag::ReadOnly)) != 0) {
      LogSkip("read-only");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    constexpr UInt32 maxBytes = 4096;
    constexpr UInt32 sectorCount = 2;

    if (info.sectorSize == 0) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt32 totalBytes = info.sectorSize * sectorCount;

    if (totalBytes > maxBytes) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    if (info.sectorCount < sectorCount) {
      LogSkip("sector count");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt32 scratchLBA = info.sectorCount - sectorCount;
    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};

    if (!ReadSectors(deviceToken, scratchLBA, sectorCount, original)) {
      TEST_ASSERT(false, "floppy multi-sector read original failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x5A ^ i);
    }

    if (!WriteSectors(deviceToken, scratchLBA, sectorCount, writeData)) {
      TEST_ASSERT(false, "floppy multi-sector write failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    if (!ReadSectors(deviceToken, scratchLBA, sectorCount, verify)) {
      TEST_ASSERT(false, "floppy multi-sector verify read failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "floppy multi-sector verify mismatch");

    WriteSectors(deviceToken, scratchLBA, sectorCount, original);

    CloseDeviceToken(deviceToken, info.id);

    return match;
  }

  bool FloppyTests::TestCrossTrackWriteReadback() {
    UInt32 deviceToken = 0;
    BlockDevices::Info info {};

    if (!FindFloppyDevice(deviceToken, info)) {
      LogSkip("no device");

      return true;
    }

    if ((info.flags & static_cast<UInt32>(BlockDevices::Flag::ReadOnly)) != 0) {
      LogSkip("read-only");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    constexpr UInt32 maxBytes = 4096;
    constexpr UInt32 sectorCount = 4;
    constexpr UInt32 assumedSectorsPerTrack = 18;

    if (info.sectorSize == 0) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt32 totalBytes = info.sectorSize * sectorCount;

    if (totalBytes > maxBytes) {
      LogSkip("sector size");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    if (info.sectorCount < sectorCount) {
      LogSkip("sector count");
      CloseDeviceToken(deviceToken, info.id);

      return true;
    }

    UInt32 scratchLBA = info.sectorCount - sectorCount;
    UInt32 trackBase = (scratchLBA / assumedSectorsPerTrack)
      * assumedSectorsPerTrack;
    UInt32 desiredLBA = trackBase + (assumedSectorsPerTrack - 2);

    if (desiredLBA + sectorCount > info.sectorCount) {
      desiredLBA = scratchLBA;
    }

    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};

    if (!ReadSectors(deviceToken, desiredLBA, sectorCount, original)) {
      TEST_ASSERT(false, "floppy cross-track read original failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x3C ^ i);
    }

    if (!WriteSectors(deviceToken, desiredLBA, sectorCount, writeData)) {
      TEST_ASSERT(false, "floppy cross-track write failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    if (!ReadSectors(deviceToken, desiredLBA, sectorCount, verify)) {
      TEST_ASSERT(false, "floppy cross-track verify read failed");
      CloseDeviceToken(deviceToken, info.id);

      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    TEST_ASSERT(match, "floppy cross-track verify mismatch");

    WriteSectors(deviceToken, desiredLBA, sectorCount, original);

    CloseDeviceToken(deviceToken, info.id);

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


