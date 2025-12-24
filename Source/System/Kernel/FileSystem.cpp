/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/FileSystem.cpp
 * Kernel filesystem service routing.
 */

#include "FileSystem.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel {
  using IPC = Kernel::IPC;
  using Task = Kernel::Task;

  FileSystem::Service* FileSystem::FindService(FileSystem::Type type) {
    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId != 0 && _services[i].type == type) {
        return &_services[i];
      }
    }

    return nullptr;
  }

  FileSystem::Service* FileSystem::FindFirstService() {
    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId != 0) {
        return &_services[i];
      }
    }

    return nullptr;
  }

  bool FileSystem::RegisterService(FileSystem::Type type, UInt32 portId) {
    if (portId == 0) {
      return false;
    }

    UInt32 ownerId = 0;

    if (!IPC::GetPortOwner(portId, ownerId)) {
      return false;
    }

    if (ownerId != Task::GetCurrentId()) {
      return false;
    }

    Service* existing = FindService(type);

    if (existing) {
      existing->portId = portId;

      return true;
    }

    for (UInt32 i = 0; i < _maxServices; ++i) {
      if (_services[i].portId == 0) {
        _services[i].type = type;
        _services[i].portId = portId;

        return true;
      }
    }

    return false;
  }

  UInt32 FileSystem::Dispatch(
    ABI::SystemCall call,
    UInt32 arg0,
    UInt32 arg1,
    UInt32 arg2
  ) {
    Service* service = FindService(Type::FAT12);

    if (!service) {
      service = FindFirstService();
    }

    if (!service) {
      return 1;
    }

    UInt32 replyPortId = IPC::CreatePort();

    if (replyPortId == 0) {
      return 1;
    }

    ServiceMessage msg {};

    msg.op = static_cast<UInt32>(call);
    msg.status = 1;
    msg.replyPortId = replyPortId;
    msg.arg0 = arg0;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.dataLength = 0;

    bool sent = IPC::Send(
      service->portId,
      Task::GetCurrentId(),
      &msg,
      messageHeaderBytes
    );

    if (!sent) {
      IPC::DestroyPort(replyPortId);

      return 1;
    }

    ServiceMessage response{};
    UInt32 senderId = 0;
    UInt32 length = 0;

    bool received = IPC::Receive(
      replyPortId,
      senderId,
      &response,
      sizeof(response),
      length
    );

    IPC::DestroyPort(replyPortId);

    (void)senderId;
    (void)length;

    if (!received) {
      return 1;
    }

    return response.status;
  }
}
