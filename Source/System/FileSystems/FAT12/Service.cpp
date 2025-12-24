/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Service.cpp
 * FAT12 filesystem service.
 */

#include <ABI/Console.hpp>
#include <ABI/FileSystem.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "Service.hpp"

namespace Quantum::System::FileSystems::FAT12 {
  using Console = ABI::Console;
  using FileSystem = ABI::FileSystem;
  using IPC = ABI::IPC;
  using Task = ABI::Task;

  void Service::Main() {
    UInt32 portId = IPC::CreatePort();

    if (portId == 0) {
      Console::WriteLine("FAT12: failed to create IPC port");
      Task::Exit(1);
    }

    UInt32 reg = FileSystem::RegisterService(
      FileSystem::Type::FAT12,
      portId
    );

    if (reg != 0) {
      Console::WriteLine("FAT12: failed to register service");
      Task::Exit(1);
    }

    Console::WriteLine("FAT12 service ready");

    for (;;) {
      IPC::Message msg {};

      if (IPC::Receive(portId, msg) != 0) {
        Task::Yield();
        continue;
      }

      if (msg.length < FileSystem::messageHeaderBytes) {
        continue;
      }

      FileSystem::ServiceMessage request {};

      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      if (request.replyPortId == 0) {
        continue;
      }

      FileSystem::ServiceMessage response {};

      response.op = request.op;
      response.status = 1;
      response.replyPortId = request.replyPortId;
      response.arg0 = 0;
      response.arg1 = 0;
      response.arg2 = 0;
      response.dataLength = 0;

      IPC::Message reply {};

      reply.length = FileSystem::messageHeaderBytes;

      if (reply.length > IPC::maxPayloadBytes) {
        continue;
      }

      for (UInt32 i = 0; i < reply.length; ++i) {
        reply.payload[i] = reinterpret_cast<UInt8*>(&response)[i];
      }

      IPC::Send(request.replyPortId, reply);
    }
  }
}
