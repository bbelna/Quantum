/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.cpp
 * User-mode floppy driver entry.
 */

#include <ABI/Block.hpp>
#include <ABI/IPC.hpp>
#include <ABI/IO.hpp>
#include <Console.hpp>
#include <Task.hpp>

#include "Driver.hpp"

namespace Quantum::System::Drivers::Storage::Floppy {
  using Block = ABI::Devices::Block;
  using IPC = ABI::IPC;
  using IO = ABI::IO;

  static constexpr UInt16 kPortDOR = 0x3F2;
  static constexpr UInt16 kPortMSR = 0x3F4;
  static constexpr UInt16 kPortFIFO = 0x3F5;

  static constexpr UInt8 kMSR_RQM = 0x80;
  static constexpr UInt8 kMSR_DIO = 0x40;

  static bool g_initialized = false;
  static IPC::Message g_inbox{};
  static IPC::Message g_outbox{};
  static Block::Message g_request{};
  static Block::Message g_response{};

  static bool WaitForReady(bool readPhase) {
    const UInt32 maxSpins = 100000;

    for (UInt32 i = 0; i < maxSpins; ++i) {
      UInt8 status = IO::In8(kPortMSR);
      bool rqm = (status & kMSR_RQM) != 0;
      bool dio = (status & kMSR_DIO) != 0;

      if (rqm && dio == readPhase) {
        return true;
      }

      if ((i & 0x3FF) == 0) {
        Task::Yield();
      }
    }

    return false;
  }

  static bool WriteData(UInt8 value) {
    if (!WaitForReady(false)) {
      return false;
    }

    IO::Out8(kPortFIFO, value);
    return true;
  }

  static bool ReadData(UInt8& value) {
    if (!WaitForReady(true)) {
      return false;
    }

    value = IO::In8(kPortFIFO);
    return true;
  }

  static bool SenseInterruptStatus(UInt8& st0, UInt8& cyl) {
    if (!WriteData(0x08)) {
      return false;
    }

    if (!ReadData(st0)) {
      return false;
    }

    if (!ReadData(cyl)) {
      return false;
    }

    return true;
  }

  static bool ResetController() {
    IO::Out8(kPortDOR, 0x00);
    IO::Out8(kPortDOR, 0x0C);

    UInt8 st0 = 0;
    UInt8 cyl = 0;

    for (UInt32 i = 0; i < 4; ++i) {
      if (!SenseInterruptStatus(st0, cyl)) {
        return false;
      }
    }

    return true;
  }

  static bool Specify() {
    if (!WriteData(0x03)) {
      return false;
    }

    if (!WriteData(0xDF)) {
      return false;
    }

    if (!WriteData(0x02)) {
      return false;
    }

    return true;
  }
  static void CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

  void Driver::Main() {
    Console::WriteLine("Floppy driver starting (stub)");

    UInt32 deviceId = 0;
    UInt32 count = Block::GetCount();

    for (UInt32 i = 1; i <= count; ++i) {
      Block::Info info{};

      if (Block::GetInfo(i, info) != 0) {
        continue;
      }

      if (info.type == Block::TypeFloppy) {
        deviceId = info.id;

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

    if (!ResetController()) {
      Console::WriteLine("Floppy controller reset failed");
      g_initialized = false;
    } else if (!Specify()) {
      Console::WriteLine("Floppy controller specify failed");
      g_initialized = false;
    } else {
      Console::WriteLine("Floppy controller initialized");
      g_initialized = true;
    }

    Console::WriteLine("Floppy driver bound to block device");

    for (;;) {
      IPC::Message& msg = g_inbox;

      if (IPC::Receive(portId, msg) != 0) {
        Task::Yield();

        continue;
      }

      if (msg.length < Block::messageHeaderBytes) {
        continue;
      }

      Block::Message& request = g_request;
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(Block::Message)) {
        copyBytes = sizeof(Block::Message);
      }

      CopyBytes(&request, msg.payload, copyBytes);

      if (request.replyPortId == 0) {
        continue;
      }

      Block::Message& response = g_response;

      response.op = Block::OpResponse;
      response.deviceId = request.deviceId;
      response.lba = request.lba;
      response.count = request.count;
      response.replyPortId = request.replyPortId;
      response.status = g_initialized ? 2 : 1;
      response.dataLength = 0;

      IPC::Message& reply = g_outbox;

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
