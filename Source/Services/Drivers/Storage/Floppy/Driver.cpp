/**
 * @file Services/Drivers/Storage/Floppy/Driver.cpp
 * @brief Floppy driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <ABI/Console.hpp>
#include <ABI/Coordinator.hpp>
#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/Handle.hpp>
#include <ABI/IO.hpp>
#include <ABI/IRQ.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>
#include <Bytes.hpp>

#include "Driver.hpp"

namespace Quantum::Services::Drivers::Storage::Floppy {
  using ::Quantum::CopyBytes;
  using ABI::Console;
  using ABI::IO;
  using ABI::Task;

  bool Driver::WaitForFIFOReady(bool readPhase) {
    const UInt32 maxSpins = 100000;

    // keep spinning until the controller is ready or we time out
    for (UInt32 i = 0; i < maxSpins; ++i) {
      UInt8 status = IO::In8(_mainStatusRegisterPort);
      bool ready = (status & _mainStatusRequestMask) != 0;
      bool direction = (status & _mainStatusDirectionMask) != 0;

      if (ready && direction == readPhase) {
        return true;
      }

      if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }

    return false;
  }

  bool Driver::WaitForIOAccess() {
    const UInt32 maxSpins = 100000;

    // keep spinning until we can access I/O or we time out
    for (UInt32 i = 0; i < maxSpins; ++i) {
      if (IO::Out8(_ioAccessProbePort, 0) == 0) {
        return true;
      }

      if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }

    return false;
  }

  UInt8 Driver::ReadCMOS(UInt8 reg) {
    // preserve NMI disable bit while selecting the register
    IO::Out8(_cmosAddressPort, static_cast<UInt8>(reg | 0x80));

    return IO::In8(_cmosDataPort);
  }

  bool Driver::WriteFIFOByte(UInt8 value) {
    if (!WaitForFIFOReady(false)) {
      return false;
    }

    IO::Out8(_dataFIFOPort, value);

    return true;
  }

  bool Driver::ReadFIFOByte(UInt8& value) {
    if (!WaitForFIFOReady(true)) {
      return false;
    }

    value = IO::In8(_dataFIFOPort);

    return true;
  }

  bool Driver::SenseInterruptStatus(UInt8& st0, UInt8& cyl) {
    if (!WriteFIFOByte(0x08)) {
      return false;
    }

    if (!ReadFIFOByte(st0)) {
      return false;
    }

    if (!ReadFIFOByte(cyl)) {
      return false;
    }

    return true;
  }

  bool Driver::ResetController() {
    // pulse reset line
    IO::Out8(_digitalOutputRegisterPort, 0x00);
    IO::Out8(_digitalOutputRegisterPort, 0x0C);

    UInt8 st0 = 0;
    UInt8 cyl = 0;

    // read interrupt status up to 4 times to clear any pending interrupts
    for (UInt32 i = 0; i < 4; ++i) {
      if (!SenseInterruptStatus(st0, cyl)) {
        return false;
      }
    }

    return true;
  }

  bool Driver::SendSpecifyCommand() {
    if (!WriteFIFOByte(0x03)) {
      return false;
    }

    if (!WriteFIFOByte(0xDF)) {
      return false;
    }

    if (!WriteFIFOByte(0x02)) {
      return false;
    }

    return true;
  }

  void Driver::FillBytes(void* dest, UInt8 value, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = value;
    }
  }

  void Driver::WriteHexByte(UInt8 value) {
    const char digits[] = "0123456789ABCDEF";
    char out[5] = {};

    out[0] = '0';
    out[1] = 'x';
    out[2] = digits[(value >> 4) & 0x0F];
    out[3] = digits[value & 0x0F];
    out[4] = '\0';

    Console::Write(out);
  }

  void Driver::WriteDecUInt(UInt32 value) {
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

  void Driver::LogResultBytes(const UInt8* result) {
    Console::Write("FDC result: ");

    for (UInt32 i = 0; i < 7; ++i) {
      WriteHexByte(result[i]);

      if (i != 6) {
        Console::Write(" ");
      }
    }

    Console::WriteLine("");
  }

  void Driver::LogReadFailure(CString message) {
    Console::Write("FDC read failed: ");
    Console::WriteLine(message);
  }

  void Driver::LogCalibrateStatus(UInt32 attempt, UInt8 st0, UInt8 cyl) {
    Console::Write("FDC calibrate attempt ");
    WriteDecUInt(attempt);
    Console::Write(": st0=");
    WriteHexByte(st0);
    Console::Write(" cyl=");
    WriteHexByte(cyl);
    Console::WriteLine("");
  }

  void Driver::CopyMessageBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

  bool Driver::IsIRQMessage(const IPC::Message& msg) {
    if (msg.length < sizeof(ABI::IRQ::Message)) {
      return false;
    }

    ABI::IRQ::Message header {};
    UInt32 copyBytes = msg.length;

    if (copyBytes > sizeof(ABI::IRQ::Message)) {
      copyBytes = sizeof(ABI::IRQ::Message);
    }

    CopyMessageBytes(&header, msg.payload, copyBytes);

    return header.op == 0 && header.irq == _irqLine;
  }

  void Driver::QueuePendingMessage(const IPC::Message& msg) {
    if (_pendingCount >= _maxPendingMessages) {
      return;
    }

    _pendingMessages[_pendingCount++] = msg;
  }

  bool Driver::WaitForIRQ() {
    const UInt32 maxSpins = 200000;

    for (UInt32 i = 0; i < maxSpins; ++i) {
      if (_irqPendingCount > 0) {
        --_irqPendingCount;

        return true;
      }

      if (_portId != 0) {
        IPC::Message msg {};

        // poll the port so we can time out if no IRQ arrives
        if (_portHandle != 0 && IPC::TryReceive(_portHandle, msg) == 0) {
          if (IsIRQMessage(msg)) {
            return true;
          }

          QueuePendingMessage(msg);
        }
      }

      if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }

    Console::WriteLine("FDC IRQ timeout");

    return false;
  }

  void Driver::RegisterIRQRoute(UInt32 portId) {
    ABI::IRQ::Handle handle = 0;
    UInt32 status = ABI::IRQ::Register(_irqLine, portId, &handle);

    if (status != 0) {
      Console::WriteLine("Floppy driver IRQ register failed");

      if (handle != 0) {
        ABI::Handle::Close(handle);
      }

      return;
    }

    _irqHandle = handle;

    if (_irqHandle != 0) {
      ABI::IRQ::Enable(_irqHandle);
    }
  }

  void Driver::SendReadySignal(UInt8 deviceTypeId) {
    ABI::Coordinator::ReadyMessage ready {};
    IPC::Message msg {};

    ready.deviceId = deviceTypeId;
    ready.state = 1;

    msg.length = sizeof(ready);

    CopyBytes(msg.payload, &ready, msg.length);

    IPC::Handle readyHandle = IPC::OpenPort(
      ABI::IPC::Ports::CoordinatorReady,
      IPC::RightSend
    );

    if (readyHandle == 0) {
      return;
    }

    IPC::Send(readyHandle, msg);
    IPC::CloseHandle(readyHandle);
  }

  bool Driver::GetDeviceInfo(
    UInt32& deviceId,
    BlockDevices::Info& info,
    UInt8& driveIndex,
    UInt32& sectorSize,
    UInt32& sectorCount,
    UInt8& sectorsPerTrack,
    UInt8& headCount
  ) {
    if (_deviceCount == 0) {
      return false;
    }

    deviceId = _deviceIds[0];
    driveIndex = _deviceIndices[0];
    sectorSize = _deviceSectorSizes[0];
    sectorCount = _deviceSectorCounts[0];
    sectorsPerTrack = _deviceSectorsPerTrack[0];
    headCount = _deviceHeadCounts[0];

    if (deviceId == 0) {
      return false;
    }

    if (BlockDevices::GetInfo(deviceId, info) != 0) {
      return false;
    }

    return true;
  }

  bool Driver::ReadToBuffer(
    UInt8 driveIndex,
    UInt32 lba,
    UInt32 count,
    UInt32 sectorSize,
    UInt8 sectorsPerTrack,
    UInt8 headCount,
    void* outBuffer,
    UInt32 bufferBytes
  ) {
    if (!outBuffer || sectorSize == 0 || count == 0) {
      return false;
    }

    UInt32 bytes = count * sectorSize;

    if (bufferBytes < bytes) {
      return false;
    }

    if (!ReadSectors(
      driveIndex,
      lba,
      count,
      sectorSize,
      sectorsPerTrack,
      headCount
    )) {
      return false;
    }

    if (_dmaBufferVirtual == nullptr || _dmaBufferBytes < bytes) {
      return false;
    }

    CopyBytes(outBuffer, _dmaBufferVirtual, bytes);

    return true;
  }

  bool Driver::WriteFromBuffer(
    UInt8 driveIndex,
    UInt32 lba,
    UInt32 count,
    UInt32 sectorSize,
    UInt8 sectorsPerTrack,
    UInt8 headCount,
    const void* buffer,
    UInt32 bufferBytes
  ) {
    if (!buffer || sectorSize == 0 || count == 0) {
      return false;
    }

    UInt32 bytes = count * sectorSize;

    if (bufferBytes < bytes) {
      return false;
    }

    if (_dmaBufferVirtual == nullptr || _dmaBufferBytes < bytes) {
      return false;
    }

    CopyBytes(_dmaBufferVirtual, buffer, bytes);

    return WriteSectors(
      driveIndex,
      lba,
      count,
      sectorSize,
      sectorsPerTrack,
      headCount
    );
  }

  bool Driver::ProgramDMARead(UInt32 physicalAddress, UInt32 lengthBytes) {
    if (lengthBytes == 0 || lengthBytes > 0x10000) {
      return false;
    }

    UInt32 endAddress = physicalAddress + lengthBytes - 1;

    if ((physicalAddress & 0xFFFF0000) != (endAddress & 0xFFFF0000)) {
      return false;
    }

    IO::Out8(_dmaMaskPort, 0x06);
    IO::Out8(_dmaClearPort, 0x00);
    IO::Out8(_dmaModePort, _dmaModeRead);
    IO::Out8(
      _dmaChannel2AddressPort,
      static_cast<UInt8>(physicalAddress & 0xFF)
    );
    IO::Out8(
      _dmaChannel2AddressPort,
      static_cast<UInt8>((physicalAddress >> 8) & 0xFF)
    );
    IO::Out8(
      _dmaChannel2PagePort,
      static_cast<UInt8>((physicalAddress >> 16) & 0xFF)
    );

    UInt32 count = lengthBytes - 1;

    IO::Out8(_dmaChannel2CountPort, static_cast<UInt8>(count & 0xFF));
    IO::Out8(_dmaChannel2CountPort, static_cast<UInt8>((count >> 8) & 0xFF));
    IO::Out8(_dmaMaskPort, 0x02);

    return true;
  }

  bool Driver::ProgramDMAWrite(UInt32 physicalAddress, UInt32 lengthBytes) {
    if (lengthBytes == 0 || lengthBytes > 0x10000) {
      return false;
    }

    UInt32 endAddress = physicalAddress + lengthBytes - 1;

    if ((physicalAddress & 0xFFFF0000) != (endAddress & 0xFFFF0000)) {
      return false;
    }

    IO::Out8(_dmaMaskPort, 0x06);
    IO::Out8(_dmaClearPort, 0x00);
    IO::Out8(_dmaModePort, _dmaModeWrite);
    IO::Out8(
      _dmaChannel2AddressPort,
      static_cast<UInt8>(physicalAddress & 0xFF)
    );
    IO::Out8(
      _dmaChannel2AddressPort,
      static_cast<UInt8>((physicalAddress >> 8) & 0xFF)
    );
    IO::Out8(
      _dmaChannel2PagePort,
      static_cast<UInt8>((physicalAddress >> 16) & 0xFF)
    );

    UInt32 count = lengthBytes - 1;

    IO::Out8(_dmaChannel2CountPort, static_cast<UInt8>(count & 0xFF));
    IO::Out8(_dmaChannel2CountPort, static_cast<UInt8>((count >> 8) & 0xFF));
    IO::Out8(_dmaMaskPort, 0x02);

    return true;
  }

  void Driver::SetDrive(UInt8 driveIndex, bool motorOn) {
    UInt8 motorMask = driveIndex == 0 ? _dorMotorA : _dorMotorB;
    UInt8 value = _dorEnableMask | (driveIndex & 0x03);

    if (motorOn) {
      value |= motorMask;
    }

    IO::Out8(_digitalOutputRegisterPort, value);

    if (driveIndex < _maxDevices) {
      _motorOn[driveIndex] = motorOn;
      _motorIdleCount[driveIndex] = 0;
    }
  }

  void Driver::WaitForMotorSpinUp() {
    const UInt32 maxSpins = 20000;

    for (UInt32 i = 0; i < maxSpins; ++i) {
      IO::Out8(_ioAccessProbePort, 0);

      if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }
  }

  void Driver::UpdateMotorIdle() {
    for (UInt32 i = 0; i < _maxDevices; ++i) {
      if (!_motorOn[i]) {
        continue;
      }

      if (++_motorIdleCount[i] < _motorIdleThreshold) {
        continue;
      }

      SetDrive(static_cast<UInt8>(i), false);
    }
  }

  bool Driver::Calibrate(UInt8 driveIndex) {
    SetDrive(driveIndex, true);
    WaitForMotorSpinUp();

    for (UInt32 attempt = 0; attempt < 5; ++attempt) {
      _irqPendingCount = 0;

      if (!WriteFIFOByte(_commandRecalibrate)) {
        return false;
      }

      if (!WriteFIFOByte(driveIndex & 0x03)) {
        return false;
      }

      if (!WaitForIRQ()) {
        continue;
      }

      UInt8 st0 = 0;
      UInt8 cyl = 0;

      if (!SenseInterruptStatus(st0, cyl)) {
        continue;
      }

      LogCalibrateStatus(attempt, st0, cyl);

      if ((st0 & 0xC0) == 0 && cyl == 0) {
        _currentCylinder[driveIndex] = 0;

        return true;
      }
    }

    return false;
  }

  bool Driver::Seek(UInt8 driveIndex, UInt8 cylinder, UInt8 head) {
    _irqPendingCount = 0;

    if (!WriteFIFOByte(_commandSeek)) {
      return false;
    }

    UInt8 driveHead
      = static_cast<UInt8>(((head & 0x01) << 2)
      | (driveIndex & 0x03));

    if (!WriteFIFOByte(driveHead)) {
      return false;
    }

    if (!WriteFIFOByte(cylinder)) {
      return false;
    }

    if (!WaitForIRQ()) {
      return false;
    }

    UInt8 st0 = 0;
    UInt8 cyl = 0;

    if (!SenseInterruptStatus(st0, cyl)) {
      return false;
    }

    if ((st0 & 0xC0) != 0 || cyl != cylinder) {
      return false;
    }

    _currentCylinder[driveIndex] = cylinder;

    return true;
  }

  void Driver::LBAToCHS(
    UInt32 lba,
    UInt8 sectorsPerTrack,
    UInt8 headCount,
    UInt8& cylinder,
    UInt8& head,
    UInt8& sector
  ) {
    if (sectorsPerTrack == 0 || headCount == 0) {
      cylinder = 0;
      head = 0;
      sector = 1;

      return;
    }

    UInt32 track = lba / sectorsPerTrack;

    sector = static_cast<UInt8>((lba % sectorsPerTrack) + 1);
    head = static_cast<UInt8>(track % headCount);
    cylinder = static_cast<UInt8>(track / headCount);
  }

  bool Driver::ReadSectors(
    UInt8 driveIndex,
    UInt32 lba,
    UInt32 count,
    UInt32 sectorSize,
    UInt8 sectorsPerTrack,
    UInt8 headCount
  ) {
    if (count == 0 || sectorSize == 0) {
      LogReadFailure("bad request");

      return false;
    }

    if (_dmaBufferVirtual == nullptr || _dmaBufferBytes < sectorSize) {
      LogReadFailure("DMA buffer too small");

      return false;
    }

    if (sectorsPerTrack == 0 || headCount == 0) {
      LogReadFailure("bad geometry");

      return false;
    }

    UInt32 remaining = count;
    UInt32 currentLBA = lba;

    SetDrive(driveIndex, true);
    WaitForMotorSpinUp();

    if (_currentCylinder[driveIndex] == 0xFF) {
      if (!Calibrate(driveIndex)) {
        LogReadFailure("calibrate");

        return false;
      }
    }

    UInt32 size = sectorSize;

    if (size < 128 || (size & (size - 1)) != 0) {
      LogReadFailure("sector size");

      return false;
    }

    UInt8 sizeCode = 0;

    while (size > 128) {
      size >>= 1;
      ++sizeCode;
    }

    while (remaining > 0) {
      UInt8 cylinder = 0;
      UInt8 head = 0;
      UInt8 sector = 0;

      LBAToCHS(currentLBA, sectorsPerTrack, headCount, cylinder, head, sector);

      UInt32 totalLeft
        = static_cast<UInt32>((sectorsPerTrack * headCount)
        - (head * sectorsPerTrack)
        - (sector - 1));
      UInt32 toRead = remaining < totalLeft ? remaining : totalLeft;
      UInt32 bytes = toRead * sectorSize;

      if (bytes > _dmaBufferBytes) {
        LogReadFailure("DMA buffer too small");

        return false;
      }

      bool success = false;
      CString failure = nullptr;

      for (UInt32 attempt = 0; attempt < _maxRetries; ++attempt) {
        failure = nullptr;
        if (attempt > 0) {
          Calibrate(driveIndex);
        }

        if (_currentCylinder[driveIndex] != cylinder) {
          if (!Seek(driveIndex, cylinder, head)) {
            failure = "seek";

            continue;
          }
        }

        if (!ProgramDMARead(_dmaBufferPhysical, bytes)) {
          failure = "DMA program";

          continue;
        }

        _irqPendingCount = 0;

        bool multiTrack
          = headCount > 1 && (sector + toRead - 1) > sectorsPerTrack;
        UInt8 command = multiTrack
          ? _commandReadDataMultiTrack
          : _commandReadData;

        if (!WriteFIFOByte(command)) {
          failure = "write command";

          continue;
        }

        UInt8 driveHead
          = static_cast<UInt8>(((head & 0x01) << 2)
          | (driveIndex & 0x03));

        if (!WriteFIFOByte(driveHead)) {
          failure = "write drive/head";

          continue;
        }

        if (!WriteFIFOByte(cylinder)) {
          failure = "write cylinder";

          continue;
        }

        if (!WriteFIFOByte(head)) {
          failure = "write head";

          continue;
        }

        if (!WriteFIFOByte(sector)) {
          failure = "write sector";

          continue;
        }

        if (!WriteFIFOByte(sizeCode)) {
          failure = "write sector size";

          continue;
        }

        UInt8 endOfTrack = multiTrack
          ? sectorsPerTrack
          : static_cast<UInt8>(sector + toRead - 1);

        if (!WriteFIFOByte(endOfTrack)) {
          failure = "write EOT";

          continue;
        }

        if (!WriteFIFOByte(0x1B)) {
          failure = "write GAP";

          continue;
        }

        if (!WriteFIFOByte(0xFF)) {
          failure = "write DTL";

          continue;
        }

        if (!WaitForIRQ()) {
          failure = "IRQ timeout";

          continue;
        }

        UInt8 result[7] = {};

        for (UInt32 i = 0; i < 7; ++i) {
          if (!ReadFIFOByte(result[i])) {
            failure = "read result";

            break;
          }
        }

        if (failure != nullptr) {
          continue;
        }

        if ((result[0] & 0xC0) != 0) {
          LogResultBytes(result);

          failure = "status error";

          continue;
        }

        success = true;

        break;
      }

      if (!success) {
        if (failure != nullptr) {
          LogReadFailure(failure);
        }

        return false;
      }

      remaining -= toRead;
      currentLBA += toRead;
    }

    return true;
  }

  bool Driver::WriteSectors(
    UInt8 driveIndex,
    UInt32 lba,
    UInt32 count,
    UInt32 sectorSize,
    UInt8 sectorsPerTrack,
    UInt8 headCount
  ) {
    if (count == 0 || sectorSize == 0) {
      LogReadFailure("bad request");

      return false;
    }

    if (sectorsPerTrack == 0 || headCount == 0) {
      LogReadFailure("bad geometry");

      return false;
    }

    if (_dmaBufferVirtual == nullptr || _dmaBufferBytes < sectorSize) {
      LogReadFailure("DMA buffer too small");

      return false;
    }

    SetDrive(driveIndex, true);
    WaitForMotorSpinUp();

    if (_currentCylinder[driveIndex] == 0xFF) {
      if (!Calibrate(driveIndex)) {
        LogReadFailure("calibrate");

        return false;
      }
    }

    UInt32 size = sectorSize;

    if (size < 128 || (size & (size - 1)) != 0) {
      LogReadFailure("sector size");

      return false;
    }

    UInt8 sizeCode = 0;

    while (size > 128) {
      size >>= 1;
      ++sizeCode;
    }

    UInt32 remaining = count;
    UInt32 currentLBA = lba;

    while (remaining > 0) {
      UInt8 cylinder = 0;
      UInt8 head = 0;
      UInt8 sector = 0;

      LBAToCHS(currentLBA, sectorsPerTrack, headCount, cylinder, head, sector);

      UInt32 totalLeft
        = static_cast<UInt32>((sectorsPerTrack * headCount)
        - (head * sectorsPerTrack)
        - (sector - 1));
      UInt32 toWrite = remaining < totalLeft ? remaining : totalLeft;
      UInt32 bytes = toWrite * sectorSize;

      if (bytes > _dmaBufferBytes) {
        LogReadFailure("DMA buffer too small");

        return false;
      }

      bool success = false;
      CString failure = nullptr;

      for (UInt32 attempt = 0; attempt < _maxRetries; ++attempt) {
        failure = nullptr;
        if (attempt > 0) {
          Calibrate(driveIndex);
        }

        if (_currentCylinder[driveIndex] != cylinder) {
          if (!Seek(driveIndex, cylinder, head)) {
            failure = "seek";

            continue;
          }
        }

        if (!ProgramDMAWrite(_dmaBufferPhysical, bytes)) {
          failure = "DMA program";

          continue;
        }

        _irqPendingCount = 0;

        bool multiTrack
          = headCount > 1 && (sector + toWrite - 1) > sectorsPerTrack;
        UInt8 command = multiTrack
          ? _commandWriteDataMultiTrack
          : _commandWriteData;

        if (!WriteFIFOByte(command)) {
          failure = "write command";

          continue;
        }

        UInt8 driveHead
          = static_cast<UInt8>(((head & 0x01) << 2)
          | (driveIndex & 0x03));

        if (!WriteFIFOByte(driveHead)) {
          failure = "write drive/head";

          continue;
        }

        if (!WriteFIFOByte(cylinder)) {
          failure = "write cylinder";

          continue;
        }

        if (!WriteFIFOByte(head)) {
          failure = "write head";

          continue;
        }

        if (!WriteFIFOByte(sector)) {
          failure = "write sector";

          continue;
        }

        if (!WriteFIFOByte(sizeCode)) {
          failure = "write sector size";

          continue;
        }

        UInt8 endOfTrack = multiTrack
          ? sectorsPerTrack
          : static_cast<UInt8>(sector + toWrite - 1);

        if (!WriteFIFOByte(endOfTrack)) {
          failure = "write EOT";

          continue;
        }

        if (!WriteFIFOByte(0x1B)) {
          failure = "write GAP";

          continue;
        }

        if (!WriteFIFOByte(0xFF)) {
          failure = "write DTL";

          continue;
        }

        if (!WaitForIRQ()) {
          failure = "IRQ timeout";

          continue;
        }

        UInt8 result[7] = {};

        for (UInt32 i = 0; i < 7; ++i) {
          if (!ReadFIFOByte(result[i])) {
            failure = "read result";

            break;
          }
        }

        if (failure != nullptr) {
          continue;
        }

        if ((result[0] & 0xC0) != 0) {
          LogResultBytes(result);

          failure = "status error";

          continue;
        }

        success = true;

        break;
      }

      if (!success) {
        if (failure != nullptr) {
          LogReadFailure(failure);
        }

        return false;
      }

      remaining -= toWrite;
      currentLBA += toWrite;
    }

    return true;
  }

  bool Driver::RegisterDevice(
    const BlockDevices::Info& info,
    UInt8 sectorsPerTrack,
    UInt8 headCount
  ) {
    if (_deviceCount >= _maxDevices) {
      return false;
    }

    UInt32 deviceId = info.id;
    UInt32 sectorSize = info.sectorSize != 0 ? info.sectorSize : 512;
    UInt8 driveIndex = static_cast<UInt8>(info.deviceIndex);

    if (driveIndex >= _maxDevices || deviceId == 0) {
      return false;
    }

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_deviceIds[i] == deviceId || _deviceIndices[i] == driveIndex) {
        return false;
      }
    }

    _deviceIds[_deviceCount] = deviceId;
    _deviceSectorSizes[_deviceCount] = sectorSize;

    UInt8 spt = sectorsPerTrack != 0
      ? sectorsPerTrack
      : _defaultSectorsPerTrack;
    UInt8 heads = headCount != 0 ? headCount : _defaultHeadCount;

    _deviceSectorCounts[_deviceCount] = info.sectorCount;
    _deviceSectorsPerTrack[_deviceCount] = spt;
    _deviceHeadCounts[_deviceCount] = heads;
    _deviceIndices[_deviceCount] = driveIndex;
    _deviceCount++;

    return true;
  }

  bool Driver::FindDevice(
    UInt32 deviceId,
    UInt8& driveIndex,
    UInt32& sectorSize,
    UInt32& sectorCount,
    UInt8& sectorsPerTrack,
    UInt8& headCount
  ) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_deviceIds[i] == deviceId) {
        driveIndex = _deviceIndices[i];
        sectorSize = _deviceSectorSizes[i];
        sectorCount = _deviceSectorCounts[i];
        sectorsPerTrack = _deviceSectorsPerTrack[i];
        headCount = _deviceHeadCounts[i];

        return true;
      }
    }

    return false;
  }

  UInt16 Driver::ReadUInt16(const UInt8* base, UInt32 offset) {
    return static_cast<UInt16>(
      base[offset] | (static_cast<UInt16>(base[offset + 1]) << 8)
    );
  }

  UInt32 Driver::ReadUInt32(const UInt8* base, UInt32 offset) {
    return static_cast<UInt32>(base[offset])
      | (static_cast<UInt32>(base[offset + 1]) << 8)
      | (static_cast<UInt32>(base[offset + 2]) << 16)
      | (static_cast<UInt32>(base[offset + 3]) << 24);
  }

  bool Driver::DetectGeometry(
    UInt8 driveIndex,
    UInt32& outSectorSize,
    UInt8& outSectorsPerTrack,
    UInt8& outHeadCount,
    UInt32& outSectorCount
  ) {
    if (_dmaBufferVirtual == nullptr || _dmaBufferBytes < 512) {
      return false;
    }

    if (!ReadSectors(driveIndex, 0, 1, 512, 1, 1)) {
      return false;
    }

    const UInt8* bpb = _dmaBufferVirtual;
    UInt16 bytesPerSector = ReadUInt16(bpb, 11);
    UInt16 sectorsPerTrack = ReadUInt16(bpb, 24);
    UInt16 headCount = ReadUInt16(bpb, 26);
    UInt16 totalSectors16 = ReadUInt16(bpb, 19);
    UInt32 totalSectors32 = ReadUInt32(bpb, 32);
    UInt32 totalSectors = totalSectors16 != 0 ? totalSectors16 : totalSectors32;

    if (bytesPerSector == 0 || sectorsPerTrack == 0 || headCount == 0) {
      return false;
    }

    if (totalSectors == 0) {
      return false;
    }

    if (
      bytesPerSector < 128
      || (bytesPerSector & (bytesPerSector - 1)) != 0
    ) {
      return false;
    }

    if (sectorsPerTrack > 0xFF || headCount > 0xFF) {
      return false;
    }

    outSectorSize = bytesPerSector;
    outSectorsPerTrack = static_cast<UInt8>(sectorsPerTrack & 0xFF);
    outHeadCount = static_cast<UInt8>(headCount & 0xFF);
    outSectorCount = totalSectors;

    return true;
  }

  void Driver::Main() {
    Console::WriteLine("Floppy driver starting");

    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      Console::WriteLine("Floppy driver failed to create IPC port");
      Task::Exit(1);
    }

    _portId = portId;

    RegisterIRQRoute(portId);

    _portHandle = IPC::OpenPort(
      portId,
      IPC::RightReceive | IPC::RightManage
    );

    if (_portHandle == 0) {
      Console::WriteLine("Floppy driver failed to open IPC handle");
      IPC::DestroyPort(portId);
      Task::Exit(1);
    }

    BlockDevices::DMABuffer dmaBuffer {};

    if (
      BlockDevices::AllocateDMABuffer(_dmaBufferDefaultBytes, dmaBuffer) != 0
    ) {
      Console::WriteLine("Floppy driver failed to allocate DMA buffer");
      Task::Exit(1);
    }

    _dmaBufferPhysical = dmaBuffer.physical;
    _dmaBufferVirtual = reinterpret_cast<UInt8*>(dmaBuffer.virtualAddress);
    _dmaBufferBytes = dmaBuffer.size;

    for (UInt32 i = 0; i < _maxDevices; ++i) {
      _currentCylinder[i] = 0xFF;
    }

    if (!WaitForIOAccess()) {
      Console::WriteLine("Floppy driver I/O access timeout");
      Task::Exit(1);
    }

    UInt8 cmosTypes = ReadCMOS(_cmosFloppyTypeRegister);
    UInt8 typeA = static_cast<UInt8>((cmosTypes >> 4) & 0x0F);
    UInt8 typeB = static_cast<UInt8>(cmosTypes & 0x0F);
    bool cmosKnown = cmosTypes != 0;

    if (!ResetController()) {
      Console::WriteLine("Floppy controller reset failed");

      _initialized = false;
    } else if (!SendSpecifyCommand()) {
      Console::WriteLine("Floppy controller specify failed");

      _initialized = false;
    } else {
      Console::WriteLine("Floppy controller initialized");

      _initialized = true;
    }

    if (!_initialized) {
      Console::WriteLine("Floppy controller init failed");
      Task::Exit(1);
    }

    _deviceCount = 0;

    for (UInt8 driveIndex = 0; driveIndex < _maxDevices; ++driveIndex) {
      UInt8 driveType = driveIndex == 0 ? typeA : typeB;

      if (cmosKnown && driveType == 0) {
        continue;
      }

      UInt8 sectorsPerTrack = _defaultSectorsPerTrack;
      UInt8 headCount = _defaultHeadCount;
      UInt32 sectorSize = 512;
      UInt32 sectorCount = 0;

      if (
        !DetectGeometry(
          driveIndex,
          sectorSize,
          sectorsPerTrack,
          headCount,
          sectorCount
        )
      ) {
        continue;
      }

      BlockDevices::Info info {};

      info.id = 0;
      info.type = BlockDevices::Type::Floppy;
      info.sectorSize = sectorSize;
      info.sectorCount = sectorCount;
      info.flags = BlockDevices::flagRemovable;
      info.deviceIndex = driveIndex;

      UInt32 deviceId = BlockDevices::Register(info);

      if (deviceId == 0) {
        Console::WriteLine("Floppy device registration failed");

        continue;
      }

      info.id = deviceId;

      if (!RegisterDevice(info, sectorsPerTrack, headCount)) {
        Console::WriteLine("Floppy driver skipping device");

        continue;
      }

      BlockDevices::Handle deviceHandle = BlockDevices::Open(
        deviceId,
        BlockDevices::RightRead
          | BlockDevices::RightWrite
          | BlockDevices::RightControl
          | BlockDevices::RightBind
      );

      if (_deviceCount > 0) {
        _deviceHandles[_deviceCount - 1] = deviceHandle;
      }
    }

    if (_deviceCount == 0) {
      Console::WriteLine("Floppy device not found");
      Task::Exit(1);
    }

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      UInt32 bindTarget = _deviceHandles[i] != 0
        ? _deviceHandles[i]
        : _deviceIds[i];

      if (BlockDevices::Bind(bindTarget, portId) != 0) {
        Console::WriteLine("Floppy driver failed to bind block device");
        Task::Exit(1);
      }
    }

    Console::WriteLine("Floppy driver bound to block device");
    SendReadySignal(1);

    for (;;) {
      IPC::Message& msg = _receiveMessage;

      if (_pendingCount > 0) {
        msg = _pendingMessages[0];

        for (UInt32 i = 1; i < _pendingCount; ++i) {
          _pendingMessages[i - 1] = _pendingMessages[i];
        }

        --_pendingCount;
      } else {
        if (IPC::Receive(_portHandle, msg) != 0) {
          Task::Yield();
          UpdateMotorIdle();

          continue;
        }
      }

      if (IsIRQMessage(msg)) {
        ++_irqPendingCount;

        continue;
      }

      if (msg.length < BlockDevices::messageHeaderBytes) {
        continue;
      }

      BlockDevices::Message& request = _blockRequest;
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(BlockDevices::Message)) {
        copyBytes = sizeof(BlockDevices::Message);
      }

      CopyBytes(&request, msg.payload, copyBytes);

      if (request.replyPortId == 0) {
        continue;
      }

      BlockDevices::Message& response = _blockResponse;

      response.op = BlockDevices::Operation::Response;
      response.deviceId = request.deviceId;
      response.lba = request.lba;
      response.count = request.count;
      response.replyPortId = request.replyPortId;
      response.status = _initialized ? 0 : 1;
      response.dataLength = 0;

      UInt8 driveIndex = 0;
      UInt32 sectorSize = 0;
      UInt32 sectorCount = 0;
      UInt8 sectorsPerTrack = 0;
      UInt8 headCount = 0;

      if (
        !FindDevice(
          request.deviceId,
          driveIndex,
          sectorSize,
          sectorCount,
          sectorsPerTrack,
          headCount
        )
      ) {
        response.status = 2;
      }

      (void)driveIndex;

      if (response.status == 0) {
        UInt32 bytes = request.count * sectorSize;

        if (bytes > BlockDevices::messageDataBytes) {
          response.status = 3;
        } else if (request.op == BlockDevices::Operation::Read) {
          if (_dmaBufferBytes < sectorSize) {
            response.status = 5;
          } else if (request.lba + request.count > sectorCount) {
            response.status = 7;
          } else {
            response.dataLength = bytes;

            UInt32 remaining = request.count;
            UInt32 lba = request.lba;
            UInt32 offset = 0;
            UInt32 maxPerChunk = _dmaBufferBytes / sectorSize;

            if (maxPerChunk == 0) {
              response.status = 5;
              response.dataLength = 0;
            } else {
              while (remaining > 0 && response.status == 0) {
                UInt32 toRead
                  = remaining < maxPerChunk ? remaining : maxPerChunk;

                if (
                  !ReadSectors(
                    driveIndex,
                    lba,
                    toRead,
                    sectorSize,
                    sectorsPerTrack,
                    headCount
                  )
                ) {
                  response.status = 6;
                  response.dataLength = 0;

                  break;
                }

                UInt32 bytesRead = toRead * sectorSize;

                CopyBytes(
                  response.data + offset,
                  _dmaBufferVirtual,
                  bytesRead
                );

                remaining -= toRead;
                lba += toRead;
                offset += bytesRead;
              }
            }
          }
        } else if (request.op == BlockDevices::Operation::Write) {
          if (_dmaBufferBytes < sectorSize) {
            response.status = 5;
          } else if (request.lba + request.count > sectorCount) {
            response.status = 7;
          } else {
            UInt32 remaining = request.count;
            UInt32 lba = request.lba;
            UInt32 offset = 0;
            UInt32 maxPerChunk = _dmaBufferBytes / sectorSize;

            if (maxPerChunk == 0) {
              response.status = 5;
            } else {
              while (remaining > 0 && response.status == 0) {
                UInt32 toWrite
                  = remaining < maxPerChunk ? remaining : maxPerChunk;
                UInt32 bytesToWrite = toWrite * sectorSize;

                CopyBytes(
                  _dmaBufferVirtual,
                  request.data + offset,
                  bytesToWrite
                );

                if (
                  !WriteSectors(
                    driveIndex,
                    lba,
                    toWrite,
                    sectorSize,
                    sectorsPerTrack,
                    headCount
                  )
                ) {
                  response.status = 6;
                  response.dataLength = 0;

                  break;
                }

                remaining -= toWrite;
                lba += toWrite;
                offset += bytesToWrite;
              }
            }
          }
        } else {
          response.status = 4;
        }
      }

      IPC::Message& reply = _sendMessage;

      reply.length = BlockDevices::messageHeaderBytes + response.dataLength;

      if (reply.length > IPC::maxPayloadBytes) {
        continue;
      }

      CopyBytes(reply.payload, &response, reply.length);
      IPC::Handle replyHandle = IPC::OpenPort(
        request.replyPortId,
        IPC::RightSend
      );

      if (replyHandle != 0) {
        IPC::Send(replyHandle, reply);
        IPC::CloseHandle(replyHandle);
      }
    }

    Task::Exit(0);
  }
}
