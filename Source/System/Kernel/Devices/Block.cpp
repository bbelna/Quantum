/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Devices/Block.cpp
 * Kernel block device registry and interface.
 */

#include <Devices/Block.hpp>
#include <IPC.hpp>
#include <Logger.hpp>
#include <Task.hpp>

namespace Quantum::System::Kernel::Devices {
  using LogLevel = Logger::Level;
  using IPC = ::Quantum::System::Kernel::IPC;
  using Task = ::Quantum::System::Kernel::Task;

  static bool FloppyStubRead(UInt32 lba, UInt32 count, void* buffer) {
    (void)lba;
    (void)count;
    (void)buffer;

    return false;
  }

  static bool FloppyStubWrite(UInt32 lba, UInt32 count, const void* buffer) {
    (void)lba;
    (void)count;
    (void)buffer;

    return false;
  }

  Block::Device* Block::_devices[Block::_maxDevices]
    = { nullptr };
  UInt32 Block::_deviceCount = 0;
  UInt32 Block::_nextDeviceId = 1;

  static Block::Device _floppyStubDevice = {
    Block::Info{
      0,
      Block::TypeFloppy,
      512,
      2880,
      Block::flagRemovable
    },
    0,
    &FloppyStubRead,
    &FloppyStubWrite
  };

  void Block::Initialize() {
    _deviceCount = 0;
    _nextDeviceId = 1;

    UInt32 id = Register(&_floppyStubDevice);

    if (id == 0) {
      Logger::Write(LogLevel::Warning, "Block: failed to register stub");
    } else {
      Logger::WriteFormatted(
        LogLevel::Debug,
        "Block: registered floppy stub id=%u",
        id
      );
    }
  }

  UInt32 Block::Register(Block::Device* device) {
    if (!device || _deviceCount >= _maxDevices) {
      return 0;
    }

    UInt32 id = _nextDeviceId++;

    device->info.id = id;
    device->portId = 0;
    _devices[_deviceCount++] = device;

    return id;
  }

  bool Block::Unregister(UInt32 deviceId) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_devices[i] && _devices[i]->info.id == deviceId) {
        _devices[i] = _devices[_deviceCount - 1];
        _devices[_deviceCount - 1] = nullptr;
        _deviceCount--;

        return true;
      }
    }

    return false;
  }

  UInt32 Block::GetCount() {
    return _deviceCount;
  }

  bool Block::GetInfo(UInt32 deviceId, Block::Info& outInfo) {
    Block::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    outInfo = device->info;

    return true;
  }

  bool Block::Read(const Block::Request& request) {
    Block::Device* device = Find(request.deviceId);

    if (!device) {
      return false;
    }

    if ((device->info.flags & flagReady) == 0) {
      return false;
    }

    if (!ValidateRequest(*device, request)) {
      return false;
    }

    if (device->portId != 0) {
      return SendRequest(*device, request, false);
    }

    if (!device->read) {
      return false;
    }

    return device->read(request.lba, request.count, request.buffer);
  }

  bool Block::Write(const Block::Request& request) {
    Block::Device* device = Find(request.deviceId);

    if (!device) {
      return false;
    }

    if ((device->info.flags & flagReady) == 0) {
      return false;
    }

    if ((device->info.flags & flagReadOnly) != 0) {
      return false;
    }

    if (!ValidateRequest(*device, request)) {
      return false;
    }

    if (device->portId != 0) {
      return SendRequest(*device, request, true);
    }

    if (!device->write) {
      return false;
    }

    return device->write(request.lba, request.count, request.buffer);
  }

  Block::Device* Block::Find(UInt32 deviceId) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_devices[i] && _devices[i]->info.id == deviceId) {
        return _devices[i];
      }
    }

    return nullptr;
  }

  bool Block::ValidateRequest(
    const Block::Device& device,
    const Block::Request& request
  ) {
    if (request.count == 0 || request.buffer == nullptr) {
      return false;
    }

    if (device.info.sectorSize == 0 || device.info.sectorCount == 0) {
      return false;
    }

    UInt64 start = request.lba;
    UInt64 count = request.count;
    UInt64 end = start + count;
    UInt64 max = device.info.sectorCount;

    if (start >= max || count > max || end > max) {
      return false;
    }

    return true;
  }

  bool Block::Bind(UInt32 deviceId, UInt32 portId) {
    Block::Device* device = Find(deviceId);

    if (!device || portId == 0) {
      return false;
    }

    UInt32 ownerId = 0;

    if (!IPC::GetPortOwner(portId, ownerId)) {
      return false;
    }

    if (ownerId != Task::GetCurrentId()) {
      return false;
    }

    device->portId = portId;
    device->info.flags |= flagReady;

    return true;
  }

  bool Block::SendRequest(
    Block::Device& device,
    const Block::Request& request,
    bool write
  ) {
    if (device.portId == 0) {
      return false;
    }

    UInt32 bytes = request.count * device.info.sectorSize;

    if (bytes > messageDataBytes) {
      return false;
    }

    UInt32 replyPortId = IPC::CreatePort();

    if (replyPortId == 0) {
      return false;
    }

    Message msg{};

    msg.op = write ? OpWrite : OpRead;
    msg.deviceId = request.deviceId;
    msg.lba = request.lba;
    msg.count = request.count;
    msg.replyPortId = replyPortId;
    msg.status = 0;
    msg.dataLength = write ? bytes : 0;

    if (write && bytes > 0) {
      CopyBytes(msg.data, request.buffer, bytes);
    }

    UInt32 length = messageHeaderBytes + msg.dataLength;
    bool sent = IPC::Send(
      device.portId,
      Task::GetCurrentId(),
      &msg,
      length
    );

    if (!sent) {
      IPC::DestroyPort(replyPortId);

      return false;
    }

    Message response{};

    UInt32 senderId = 0;
    UInt32 responseLength = 0;
    bool received = IPC::Receive(
      replyPortId,
      senderId,
      &response,
      IPC::maxPayloadBytes,
      responseLength
    );

    IPC::DestroyPort(replyPortId);

    (void)senderId;
    (void)responseLength;

    if (!received) {
      return false;
    }

    if (response.op != OpResponse || response.status != 0) {
      return false;
    }

    if (!write) {
      if (response.dataLength != bytes) {
        return false;
      }

      if (bytes > 0) {
        CopyBytes(request.buffer, response.data, bytes);
      }
    }

    return true;
  }

  void Block::CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

}
