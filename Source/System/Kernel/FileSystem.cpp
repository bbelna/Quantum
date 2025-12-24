/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/FileSystem.cpp
 * Kernel file system service routing.
 */

#include "FileSystem.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel {
  using IPC = Kernel::IPC;
  using Task = Kernel::Task;

  static UInt32 CopyString(CString src, UInt8* dest, UInt32 maxBytes) {
    if (!src || !dest || maxBytes == 0) {
      return 0;
    }

    UInt32 length = 0;

    while (length + 1 < maxBytes && src[length] != '\0') {
      dest[length] = static_cast<UInt8>(src[length]);
      ++length;
    }

    dest[length] = '\0';

    return length + 1;
  }

  static void CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

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
    UInt32 outputPtr = 0;
    UInt32 outputBytes = 0;

    msg.dataLength = 0; // default to no payload

    if (call == ABI::SystemCall::FileSystem_ListVolumes) {
      // caller expects an array of volume entries
      outputPtr = arg0;
      outputBytes = arg1 * static_cast<UInt32>(sizeof(VolumeEntry));
    } else if (call == ABI::SystemCall::FileSystem_GetVolumeInfo) {
      // caller expects a volume info struct
      outputPtr = arg1;
      outputBytes = static_cast<UInt32>(sizeof(VolumeInfo));
    } else if (call == ABI::SystemCall::FileSystem_OpenVolume) {
      // copy the label into the request payload
      msg.dataLength = CopyString(
        reinterpret_cast<CString>(arg0),
        msg.data,
        messageDataBytes
      );
    }

    msg.op = static_cast<UInt32>(call);
    msg.status = 1;
    msg.replyPortId = replyPortId;
    msg.arg0 = arg0;
    msg.arg1 = arg1;
    msg.arg2 = arg2;

    UInt32 messageBytes = messageHeaderBytes + msg.dataLength;
    bool sent = IPC::Send(
      service->portId,
      Task::GetCurrentId(),
      &msg,
      messageBytes
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

    if (outputPtr != 0 && outputBytes > 0 && response.dataLength > 0) {
      UInt32 copyBytes = response.dataLength;

      if (copyBytes > outputBytes) {
        copyBytes = outputBytes;
      }

      CopyBytes(reinterpret_cast<void*>(outputPtr), response.data, copyBytes);
    }

    return response.status;
  }
}
