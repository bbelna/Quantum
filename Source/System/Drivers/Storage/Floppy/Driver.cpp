/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.cpp
 * User-mode floppy driver.
 */

#include <ABI/Devices/Block.hpp>
#include <ABI/IPC.hpp>
#include <ABI/IO.hpp>
#include <Console.hpp>
#include <Task.hpp>

#include "Driver.hpp"

namespace Quantum::System::Drivers::Storage::Floppy {
  using Block = ABI::Devices::Block;
  using IPC = ABI::IPC;
  using IO = ABI::IO;

  bool Driver::_initialized = false;
  UInt32 Driver::_deviceIds[Driver::_maxDevices] = {};
  UInt32 Driver::_deviceSectorSizes[Driver::_maxDevices] = {};
  UInt8 Driver::_deviceIndices[Driver::_maxDevices] = {};
  UInt32 Driver::_deviceCount = 0;
  UInt32 Driver::_dmaBufferPhysical = 0;
  UInt8* Driver::_dmaBufferVirtual = nullptr;
  UInt32 Driver::_dmaBufferBytes = 0;
  UInt8 Driver::_currentCylinder[Driver::_maxDevices] = {};
  volatile UInt32 Driver::_irqPendingCount = 0;
  UInt32 Driver::_portId = 0;
  IPC::Message Driver::_pendingMessages[Driver::_maxPendingMessages]{};
  UInt32 Driver::_pendingCount = 0;
  IPC::Message Driver::_receiveMessage{};
  IPC::Message Driver::_sendMessage{};
  Block::Message Driver::_blockRequest{};
  Block::Message Driver::_blockResponse{};

  bool Driver::WaitForFIFOReady(bool readPhase) {
    const UInt32 maxSpins = 100000;

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
    IO::Out8(_digitalOutputRegisterPort, 0x00);
    IO::Out8(_digitalOutputRegisterPort, 0x0C);

    UInt8 st0 = 0;
    UInt8 cyl = 0;

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

  void Driver::CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

  void Driver::FillBytes(void* dest, UInt8 value, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = value;
    }
  }

  static void WriteHexByte(UInt8 value) {
    const char digits[] = "0123456789ABCDEF";
    char out[5] = {};

    out[0] = '0';
    out[1] = 'x';
    out[2] = digits[(value >> 4) & 0x0F];
    out[3] = digits[value & 0x0F];
    out[4] = '\0';

    Console::Write(out);
  }

  static void WriteDecUInt(UInt32 value) {
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

  static void LogResultBytes(const UInt8* result) {
    Console::Write("FDC result: ");

    for (UInt32 i = 0; i < 7; ++i) {
      WriteHexByte(result[i]);

      if (i != 6) {
        Console::Write(" ");
      }
    }

    Console::WriteLine("");
  }

  static void LogReadFailure(CString message) {
    Console::Write("FDC read failed: ");
    Console::WriteLine(message);
  }

  static void LogCalibrateStatus(UInt32 attempt, UInt8 st0, UInt8 cyl) {
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
    if (msg.length < Block::messageHeaderBytes) {
      return false;
    }

    Block::Message header{};
    UInt32 copyBytes = msg.length;

    if (copyBytes > sizeof(Block::Message)) {
      copyBytes = sizeof(Block::Message);
    }

    CopyMessageBytes(&header, msg.payload, copyBytes);

    return header.op == Block::Operation::Response &&
      header.replyPortId == 0;
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
        IPC::Message msg{};

        if (IPC::Receive(_portId, msg) == 0) {
          if (IsIRQMessage(msg)) {
            return true;
          }

          QueuePendingMessage(msg);
        }
      } else if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }

    Console::WriteLine("FDC IRQ timeout");

    return false;
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

  void Driver::SetDrive(UInt8 driveIndex, bool motorOn) {
    UInt8 motorMask = driveIndex == 0 ? _dorMotorA : _dorMotorB;
    UInt8 value = _dorEnableMask | (driveIndex & 0x03);

    if (motorOn) {
      value |= motorMask;
    }

    IO::Out8(_digitalOutputRegisterPort, value);
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

  void Driver::LbaToCHS(
    UInt32 lba,
    UInt8& cylinder,
    UInt8& head,
    UInt8& sector
  ) {
    UInt32 track = lba / _sectorsPerTrack;

    sector = static_cast<UInt8>((lba % _sectorsPerTrack) + 1);
    head = static_cast<UInt8>(track % _headCount);
    cylinder = static_cast<UInt8>(track / _headCount);
  }

  bool Driver::ReadSectors(
    UInt8 driveIndex,
    UInt32 lba,
    UInt32 count,
    UInt32 sectorSize
  ) {
    if (count != 1 || sectorSize == 0) {
      LogReadFailure("bad request");

      return false;
    }

    if (_dmaBufferVirtual == nullptr || _dmaBufferBytes < sectorSize) {
      LogReadFailure("DMA buffer too small");

      return false;
    }

    UInt8 cylinder = 0;
    UInt8 head = 0;
    UInt8 sector = 0;

    LbaToCHS(lba, cylinder, head, sector);
    SetDrive(driveIndex, true);
    WaitForMotorSpinUp();

    if (_currentCylinder[driveIndex] == 0xFF) {
      if (!Calibrate(driveIndex)) {
        LogReadFailure("calibrate");

        return false;
      }
    }

    if (_currentCylinder[driveIndex] != cylinder) {
      if (!Seek(driveIndex, cylinder, head)) {
        LogReadFailure("seek");

        return false;
      }
    }

    if (!ProgramDMARead(_dmaBufferPhysical, sectorSize)) {
      LogReadFailure("DMA program");

      return false;
    }

    _irqPendingCount = 0;

    if (!WriteFIFOByte(_commandReadData)) {
      LogReadFailure("write command");

      return false;
    }

    UInt8 driveHead
      = static_cast<UInt8>(((head & 0x01) << 2)
      | (driveIndex & 0x03));

    if (!WriteFIFOByte(driveHead)) {
      LogReadFailure("write drive/head");

      return false;
    }

    if (!WriteFIFOByte(cylinder)) {
      LogReadFailure("write cylinder");

      return false;
    }

    if (!WriteFIFOByte(head)) {
      LogReadFailure("write head");

      return false;
    }

    if (!WriteFIFOByte(sector)) {
      LogReadFailure("write sector");

      return false;
    }

    if (!WriteFIFOByte(0x02)) {
      LogReadFailure("write sector size");

      return false;
    }

    if (!WriteFIFOByte(_sectorsPerTrack)) {
      LogReadFailure("write EOT");

      return false;
    }

    if (!WriteFIFOByte(0x1B)) {
      LogReadFailure("write GAP");

      return false;
    }

    if (!WriteFIFOByte(0xFF)) {
      LogReadFailure("write DTL");

      return false;
    }

    if (!WaitForIRQ()) {
      LogReadFailure("IRQ timeout");

      return false;
    }

    UInt8 result[7] = {};

    for (UInt32 i = 0; i < 7; ++i) {
      if (!ReadFIFOByte(result[i])) {
        LogReadFailure("read result");

        return false;
      }
    }

    if ((result[0] & 0xC0) != 0) {
      LogResultBytes(result);
      LogReadFailure("status error");

      return false;
    }

    return true;
  }

  bool Driver::RegisterDevice(const Block::Info& info) {
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
    _deviceIndices[_deviceCount] = driveIndex;
    _deviceCount++;

    return true;
  }

  bool Driver::FindDevice(
    UInt32 deviceId,
    UInt8& driveIndex,
    UInt32& sectorSize
  ) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_deviceIds[i] == deviceId) {
        driveIndex = _deviceIndices[i];
        sectorSize = _deviceSectorSizes[i];

        return true;
      }
    }

    return false;
  }

  void Driver::Main() {
    Console::WriteLine("Floppy driver starting (stub)");

    UInt32 count = Block::GetCount();

    _deviceCount = 0;

    for (UInt32 i = 1; i <= count; ++i) {
      Block::Info info{};

      if (Block::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type == Block::Type::Floppy) {
        if (!RegisterDevice(info)) {
          Console::WriteLine("Floppy driver skipping device");
        }
      }
    }

    if (_deviceCount == 0) {
      Console::WriteLine("Floppy device not found");
      Task::Exit(1);
    }

    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      Console::WriteLine("Floppy driver failed to create IPC port");
      Task::Exit(1);
    }

    _portId = portId;

    Block::DMABuffer dmaBuffer{};

    if (Block::AllocateDMABuffer(_dmaBufferDefaultBytes, dmaBuffer) != 0) {
      Console::WriteLine("Floppy driver failed to allocate DMA buffer");
      Task::Exit(1);
    }

    _dmaBufferPhysical = dmaBuffer.physical;
    _dmaBufferVirtual = reinterpret_cast<UInt8*>(dmaBuffer.virtualAddress);
    _dmaBufferBytes = dmaBuffer.size;

    for (UInt32 i = 0; i < _maxDevices; ++i) {
      _currentCylinder[i] = 0xFF;
    }

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (Block::Bind(_deviceIds[i], portId) != 0) {
        Console::WriteLine("Floppy driver failed to bind block device");
        Task::Exit(1);
      }
    }

    if (!WaitForIOAccess()) {
      Console::WriteLine("Floppy driver I/O access timeout");
      Task::Exit(1);
    }

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

    Console::WriteLine("Floppy driver bound to block device");

    for (;;) {
      IPC::Message& msg = _receiveMessage;

      if (_pendingCount > 0) {
        msg = _pendingMessages[0];

        for (UInt32 i = 1; i < _pendingCount; ++i) {
          _pendingMessages[i - 1] = _pendingMessages[i];
        }

        --_pendingCount;
      } else {
        if (IPC::Receive(portId, msg) != 0) {
          Task::Yield();

          continue;
        }
      }

      if (msg.length < Block::messageHeaderBytes) {
        continue;
      }

      Block::Message& request = _blockRequest;
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(Block::Message)) {
        copyBytes = sizeof(Block::Message);
      }

      CopyBytes(&request, msg.payload, copyBytes);

      if (
        request.op == Block::Operation::Response &&
        request.replyPortId == 0
      ) {
        ++_irqPendingCount;

        continue;
      }

      if (request.replyPortId == 0) {
        continue;
      }

      Block::Message& response = _blockResponse;

      response.op = Block::Operation::Response;
      response.deviceId = request.deviceId;
      response.lba = request.lba;
      response.count = request.count;
      response.replyPortId = request.replyPortId;
      response.status = _initialized ? 0 : 1;
      response.dataLength = 0;

      UInt8 driveIndex = 0;
      UInt32 sectorSize = 0;

      if (!FindDevice(request.deviceId, driveIndex, sectorSize)) {
        response.status = 2;
      }

      (void)driveIndex;

      if (response.status == 0) {
        UInt32 bytes = request.count * sectorSize;

        if (bytes > Block::messageDataBytes) {
          response.status = 3;
        } else if (request.op == Block::Operation::Read) {
          if (_dmaBufferBytes < sectorSize) {
            response.status = 5;
          } else {
            response.dataLength = bytes;

            for (UInt32 i = 0; i < request.count; ++i) {
              if (!ReadSectors(driveIndex, request.lba + i, 1, sectorSize)) {
                response.status = 6;
                response.dataLength = 0;

                break;
              }

              CopyBytes(
                response.data + (i * sectorSize),
                _dmaBufferVirtual,
                sectorSize
              );
            }
          }
        } else if (request.op != Block::Operation::Write) {
          response.status = 4;
        }
      }

      IPC::Message& reply = _sendMessage;

      reply.length = Block::messageHeaderBytes + response.dataLength;

      if (reply.length > IPC::maxPayloadBytes) {
        continue;
      }

      CopyBytes(reply.payload, &response, reply.length);
      IPC::Send(request.replyPortId, reply);
    }

    Task::Exit(0);
  }
}
