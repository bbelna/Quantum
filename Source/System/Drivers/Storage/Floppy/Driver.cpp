/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.cpp
 * User-mode floppy driver entry.
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
  UInt32 Driver::_deviceId = 0;
  UInt32 Driver::_sectorSize = 512;
  IPC::Message Driver::_receiveMessage{};
  IPC::Message Driver::_sendMessage{};
  Block::Message Driver::_blockRequest{};
  Block::Message Driver::_blockResponse{};

  bool Driver::WaitForFifoReady(bool readPhase) {
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

  bool Driver::WriteFifoByte(UInt8 value) {
    if (!WaitForFifoReady(false)) {
      return false;
    }

    IO::Out8(_dataFifoPort, value);
    return true;
  }

  bool Driver::ReadFifoByte(UInt8& value) {
    if (!WaitForFifoReady(true)) {
      return false;
    }

    value = IO::In8(_dataFifoPort);
    return true;
  }

  bool Driver::SenseInterruptStatus(UInt8& st0, UInt8& cyl) {
    if (!WriteFifoByte(0x08)) {
      return false;
    }

    if (!ReadFifoByte(st0)) {
      return false;
    }

    if (!ReadFifoByte(cyl)) {
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
    if (!WriteFifoByte(0x03)) {
      return false;
    }

    if (!WriteFifoByte(0xDF)) {
      return false;
    }

    if (!WriteFifoByte(0x02)) {
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

  void Driver::Main() {
    Console::WriteLine("Floppy driver starting (stub)");

    UInt32 deviceId = 0;
    UInt32 count = Block::GetCount();
    Block::Info deviceInfo{};

    for (UInt32 i = 1; i <= count; ++i) {
      Block::Info info{};

      if (Block::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type == Block::Type::Floppy) {
        deviceId = info.id;
        deviceInfo = info;

        break;
      }
    }

    if (deviceId == 0) {
      Console::WriteLine("Floppy device not found");
      Task::Exit(1);
    }

    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      Console::WriteLine("Floppy driver failed to create IPC port");
      Task::Exit(1);
    }

    if (Block::Bind(deviceId, portId) != 0) {
      Console::WriteLine("Floppy driver failed to bind block device");
      Task::Exit(1);
    }

    _deviceId = deviceId;
    _sectorSize = deviceInfo.sectorSize != 0 ? deviceInfo.sectorSize : 512;

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

      if (IPC::Receive(portId, msg) != 0) {
        Task::Yield();

        continue;
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

      if (request.deviceId != _deviceId) {
        response.status = 2;
      }

      if (response.status == 0) {
        UInt32 bytes = request.count * _sectorSize;

        if (bytes > Block::messageDataBytes) {
          response.status = 3;
        } else if (request.op == Block::Operation::Read) {
          response.dataLength = bytes;
          FillBytes(response.data, 0, bytes);
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
