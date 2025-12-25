/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/FileSystem.cpp
 * Coordinator file system broker.
 */

#include <ABI/Console.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Task.hpp>

#include "FileSystem.hpp"

namespace Quantum::System::Coordinator {
  using Console = ABI::Console;
  using IPC = ABI::IPC;
  using Task = ABI::Task;

  void FileSystem::Initialize() {
    if (_portId != 0) {
      return;
    }

    _portId = IPC::CreatePort();

    if (_portId == 0) {
      Console::WriteLine("Coordinator: failed to create file system port");
      return;
    }

    if (_portId != IPC::Ports::FileSystem) {
      Console::WriteLine("Coordinator: file system port id mismatch");
    }
  }

  FileSystem::Service* FileSystem::FindFirstService() {
    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId != 0) {
        return &_services[i];
      }
    }

    return nullptr;
  }

  FileSystem::Service* FileSystem::FindService(
    ABI::FileSystem::Type type
  ) {
    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId != 0 && _services[i].type == type) {
        return &_services[i];
      }
    }

    return nullptr;
  }

  void FileSystem::RegisterService(
    ABI::FileSystem::Type type,
    UInt32 portId
  ) {
    if (portId == 0) {
      return;
    }

    Service* existing = FindService(type);

    if (existing) {
      existing->portId = portId;

      return;
    }

    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId == 0) {
        _services[i].type = type;
        _services[i].portId = portId;

        return;
      }
    }
  }

  void FileSystem::ProcessPending() {
    if (_portId == 0) {
      return;
    }

    for (;;) {
      IPC::Message msg {};

      if (IPC::TryReceive(_portId, msg) != 0) {
        break;
      }

      if (msg.length < ABI::FileSystem::messageHeaderBytes) {
        continue;
      }

      ABI::FileSystem::ServiceMessage request {};
      UInt32 copyBytes = msg.length;

      if (copyBytes > sizeof(request)) {
        copyBytes = sizeof(request);
      }

      for (UInt32 i = 0; i < copyBytes; ++i) {
        reinterpret_cast<UInt8*>(&request)[i] = msg.payload[i];
      }

      if (
        request.op == static_cast<UInt32>(
          ABI::FileSystem::Operation::RegisterService
        )
      ) {
        RegisterService(
          static_cast<ABI::FileSystem::Type>(request.arg0),
          request.arg1
        );

        if (request.replyPortId != 0) {
          ABI::FileSystem::ServiceMessage response {};
          IPC::Message reply {};

          response.op = request.op;
          response.status = 0;
          response.replyPortId = request.replyPortId;
          response.arg0 = 0;
          response.arg1 = 0;
          response.arg2 = 0;
          response.dataLength = 0;

          reply.length = ABI::FileSystem::messageHeaderBytes;

          for (UInt32 i = 0; i < reply.length; ++i) {
            reply.payload[i] = reinterpret_cast<UInt8*>(&response)[i];
          }

          IPC::Send(request.replyPortId, reply);
        }

        continue;
      }

      Service* service = FindFirstService();

      if (!service) {
        if (request.replyPortId != 0) {
          ABI::FileSystem::ServiceMessage response {};
          IPC::Message reply {};

          response.op = request.op;
          response.status = 1;
          response.replyPortId = request.replyPortId;
          response.arg0 = 0;
          response.arg1 = 0;
          response.arg2 = 0;
          response.dataLength = 0;

          reply.length = ABI::FileSystem::messageHeaderBytes;

          for (UInt32 i = 0; i < reply.length; ++i) {
            reply.payload[i] = reinterpret_cast<UInt8*>(&response)[i];
          }

          IPC::Send(request.replyPortId, reply);
        }

        continue;
      }

      IPC::Message forward {};

      forward.length = msg.length;

      for (UInt32 i = 0; i < msg.length; ++i) {
        forward.payload[i] = msg.payload[i];
      }

      IPC::Send(service->portId, forward);
    }

    Task::Yield();
  }
}
