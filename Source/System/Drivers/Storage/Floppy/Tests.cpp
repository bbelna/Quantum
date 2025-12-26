/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Tests.cpp
 * Floppy driver tests.
 */

#include <ABI/Console.hpp>
#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/Task.hpp>

#include "Driver.hpp"
#include "Tests.hpp"

namespace Quantum::System::Drivers::Storage::Floppy::Tests {
  using Console = ABI::Console;
  using BlockDevice = ABI::Devices::BlockDevice;
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
    Console::WriteLine("Running floppy tests...");
  }

  static void LogFooter() {
    Console::Write("Floppy tests complete: passed=");
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

    Console::Write("Floppy tests skipped (");
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

  static bool PrepareFloppy(
    UInt32& deviceId,
    BlockDevice::Info& info,
    bool requireWrite,
    UInt8& driveIndex,
    UInt32& sectorSize,
    UInt32& sectorCount,
    UInt8& sectorsPerTrack,
    UInt8& headCount
  ) {
    if (!Driver::GetDeviceInfo(
      deviceId,
      info,
      driveIndex,
      sectorSize,
      sectorCount,
      sectorsPerTrack,
      headCount
    )) {
      LogSkip("no device");

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
    UInt8 driveIndex = 0;
    UInt32 sectorSize = 0;
    UInt32 sectorCount = 0;
    UInt8 sectorsPerTrack = 0;
    UInt8 headCount = 0;

    if (!PrepareFloppy(
      deviceId,
      info,
      false,
      driveIndex,
      sectorSize,
      sectorCount,
      sectorsPerTrack,
      headCount
    )) {
      return true;
    }

    UInt8 buffer[512] = {};
    bool ok = Driver::ReadToBuffer(
      driveIndex,
      0,
      1,
      sectorSize,
      sectorsPerTrack,
      headCount,
      buffer,
      sizeof(buffer)
    );

    if (!Assert(ok, "floppy read failed")) {
      return false;
    }

    UInt32 nonZeroCount = 0;

    for (UInt32 i = 0; i < sizeof(buffer); ++i) {
      if (buffer[i] != 0) {
        nonZeroCount++;
      }
    }

    return Assert(nonZeroCount > 0, "floppy read empty data");
  }

  static bool TestMultiSectorRead() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};
    UInt8 driveIndex = 0;
    UInt32 sectorSize = 0;
    UInt32 sectorCount = 0;
    UInt8 sectorsPerTrack = 0;
    UInt8 headCount = 0;

    if (!PrepareFloppy(
      deviceId,
      info,
      false,
      driveIndex,
      sectorSize,
      sectorCount,
      sectorsPerTrack,
      headCount
    )) {
      return true;
    }

    UInt8 buffer[1024] = {};
    bool ok = Driver::ReadToBuffer(
      driveIndex,
      0,
      2,
      sectorSize,
      sectorsPerTrack,
      headCount,
      buffer,
      sizeof(buffer)
    );

    if (!Assert(ok, "floppy multi-sector read failed")) {
      return false;
    }

    UInt32 nonZeroCount = 0;

    for (UInt32 i = 0; i < sizeof(buffer); ++i) {
      if (buffer[i] != 0) {
        nonZeroCount++;
      }
    }

    return Assert(nonZeroCount > 0, "floppy multi-sector read empty data");
  }

  static bool TestWriteReadback() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};
    UInt8 driveIndex = 0;
    UInt32 sectorSize = 0;
    UInt32 sectorCount = 0;
    UInt8 sectorsPerTrack = 0;
    UInt8 headCount = 0;

    if (!PrepareFloppy(
      deviceId,
      info,
      true,
      driveIndex,
      sectorSize,
      sectorCount,
      sectorsPerTrack,
      headCount
    )) {
      return true;
    }

    if (sectorSize > 512 || sectorSize == 0) {
      LogSkip("sector size");

      return true;
    }

    UInt32 totalSectors = sectorCount;

    if (totalSectors == 0) {
      LogSkip("sector count");

      return true;
    }

    UInt32 scratchLBA = totalSectors > 1 ? totalSectors - 1 : 0;
    UInt8 original[512] = {};
    UInt8 writeData[512] = {};
    UInt8 verify[512] = {};
    bool ok = Driver::ReadToBuffer(
      driveIndex,
      scratchLBA,
      1,
      sectorSize,
      sectorsPerTrack,
      headCount,
      original,
      sizeof(original)
    );

    if (!Assert(ok, "floppy write test read original failed")) {
      return false;
    }

    for (UInt32 i = 0; i < sectorSize; ++i) {
      writeData[i] = static_cast<UInt8>(0xA5 ^ i);
    }

    ok = Driver::WriteFromBuffer(
      driveIndex,
      scratchLBA,
      1,
      sectorSize,
      sectorsPerTrack,
      headCount,
      writeData,
      sizeof(writeData)
    );

    if (!Assert(ok, "floppy write failed")) {
      return false;
    }

    ok = Driver::ReadToBuffer(
      driveIndex,
      scratchLBA,
      1,
      sectorSize,
      sectorsPerTrack,
      headCount,
      verify,
      sizeof(verify)
    );

    if (!Assert(ok, "floppy write verify read failed")) {
      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < sectorSize; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    Assert(match, "floppy write verify mismatch");

    Driver::WriteFromBuffer(
      driveIndex,
      scratchLBA,
      1,
      sectorSize,
      sectorsPerTrack,
      headCount,
      original,
      sizeof(original)
    );

    return match;
  }

  static bool TestMultiSectorWriteReadback() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};
    UInt8 driveIndex = 0;
    UInt32 sectorSize = 0;
    UInt32 sectorCountTotal = 0;
    UInt8 sectorsPerTrack = 0;
    UInt8 headCount = 0;

    if (!PrepareFloppy(
      deviceId,
      info,
      true,
      driveIndex,
      sectorSize,
      sectorCountTotal,
      sectorsPerTrack,
      headCount
    )) {
      return true;
    }

    constexpr UInt32 sectorCount = 2;
    constexpr UInt32 maxBytes = 2048;
    UInt32 totalBytes = sectorSize * sectorCount;

    if (sectorSize == 0 || totalBytes > maxBytes) {
      LogSkip("sector size");

      return true;
    }

    UInt32 totalSectors = sectorCountTotal;

    if (totalSectors < sectorCount) {
      LogSkip("sector count");

      return true;
    }

    UInt32 scratchLBA = totalSectors - sectorCount;
    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};
    bool ok = Driver::ReadToBuffer(
      driveIndex,
      scratchLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      original,
      sizeof(original)
    );

    if (!Assert(ok, "floppy multi-sector read original failed")) {
      return false;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x5A ^ i);
    }

    ok = Driver::WriteFromBuffer(
      driveIndex,
      scratchLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      writeData,
      sizeof(writeData)
    );

    if (!Assert(ok, "floppy multi-sector write failed")) {
      return false;
    }

    ok = Driver::ReadToBuffer(
      driveIndex,
      scratchLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      verify,
      sizeof(verify)
    );

    if (!Assert(ok, "floppy multi-sector verify read failed")) {
      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    Assert(match, "floppy multi-sector verify mismatch");

    Driver::WriteFromBuffer(
      driveIndex,
      scratchLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      original,
      sizeof(original)
    );

    return match;
  }

  static bool TestCrossTrackWriteReadback() {
    UInt32 deviceId = 0;
    BlockDevice::Info info {};
    UInt8 driveIndex = 0;
    UInt32 sectorSize = 0;
    UInt32 sectorCountTotal = 0;
    UInt8 sectorsPerTrack = 0;
    UInt8 headCount = 0;

    if (!PrepareFloppy(
      deviceId,
      info,
      true,
      driveIndex,
      sectorSize,
      sectorCountTotal,
      sectorsPerTrack,
      headCount
    )) {
      return true;
    }

    constexpr UInt32 sectorCount = 4;
    constexpr UInt32 maxBytes = 2048;
    constexpr UInt32 assumedSectorsPerTrack = 18;
    UInt32 totalBytes = sectorSize * sectorCount;

    if (sectorSize == 0 || totalBytes > maxBytes) {
      LogSkip("sector size");

      return true;
    }

    UInt32 totalSectors = sectorCountTotal;

    if (totalSectors < sectorCount) {
      LogSkip("sector count");

      return true;
    }

    UInt32 scratchLBA = totalSectors - sectorCount;
    UInt32 trackBase = (scratchLBA / assumedSectorsPerTrack)
      * assumedSectorsPerTrack;
    UInt32 desiredLBA = trackBase + (assumedSectorsPerTrack - 2);

    if (desiredLBA + sectorCount > totalSectors) {
      desiredLBA = scratchLBA;
    }

    UInt8 original[maxBytes] = {};
    UInt8 writeData[maxBytes] = {};
    UInt8 verify[maxBytes] = {};
    bool ok = Driver::ReadToBuffer(
      driveIndex,
      desiredLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      original,
      sizeof(original)
    );

    if (!Assert(ok, "floppy cross-track read original failed")) {
      return false;
    }

    for (UInt32 i = 0; i < totalBytes; ++i) {
      writeData[i] = static_cast<UInt8>(0x3C ^ i);
    }

    ok = Driver::WriteFromBuffer(
      driveIndex,
      desiredLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      writeData,
      sizeof(writeData)
    );

    if (!Assert(ok, "floppy cross-track write failed")) {
      return false;
    }

    ok = Driver::ReadToBuffer(
      driveIndex,
      desiredLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      verify,
      sizeof(verify)
    );

    if (!Assert(ok, "floppy cross-track verify read failed")) {
      return false;
    }

    bool match = true;

    for (UInt32 i = 0; i < totalBytes; ++i) {
      if (verify[i] != writeData[i]) {
        match = false;

        break;
      }
    }

    Assert(match, "floppy cross-track verify mismatch");

    Driver::WriteFromBuffer(
      driveIndex,
      desiredLBA,
      sectorCount,
      sectorSize,
      sectorsPerTrack,
      headCount,
      original,
      sizeof(original)
    );

    return match;
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

    RunTest("Floppy single-sector read", TestSingleSectorRead);
    RunTest("Floppy multi-sector read", TestMultiSectorRead);
    RunTest("Floppy write/readback", TestWriteReadback);
    RunTest("Floppy multi-sector write/readback", TestMultiSectorWriteReadback);
    RunTest("Floppy cross-track write/readback", TestCrossTrackWriteReadback);

    LogFooter();
  }
}
