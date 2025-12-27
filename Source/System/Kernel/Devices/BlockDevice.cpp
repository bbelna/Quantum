/**
 * @file System/Kernel/Devices/BlockDevice.cpp
 * @brief Block device registry and I/O interface.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#include "Arch/Memory.hpp"
#include "Devices/BlockDevice.hpp"
#include "IPC.hpp"
#include "Logger.hpp"
#include "Memory.hpp"
#include "Task.hpp"

namespace Quantum::System::Kernel::Devices {
  using IPC = Kernel::IPC;
  using LogLevel = Kernel::Logger::Level;
  using Memory = Kernel::Memory;
  using Task = Kernel::Task;

  void BlockDevice::Initialize() {
    _deviceCount = 0;
    _nextDeviceId = 1;

    for (UInt32 i = 0; i < _maxDevices; ++i) {
      _devices[i] = nullptr;
      _deviceStorage[i].info.id = 0;
      _deviceStorage[i].portId = 0;
    }
  }

  void BlockDevice::NotifyIRQ(Type type) {
    Message msg {};

    msg.op = Operation::Response;
    msg.lba = 0;
    msg.count = 0;
    msg.replyPortId = 0;
    msg.status = 0;
    msg.dataLength = 0;

    UInt32 senderId = Task::GetCurrentId();

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      Device* device = _devices[i];

      if (
        !device
        || device->portId == 0
        || device->info.type != type
      ) {
        continue;
      }

      msg.deviceId = device->info.id;

      IPC::Send(device->portId, senderId, &msg, messageHeaderBytes);
    }
  }

  bool BlockDevice::AllocateDMABuffer(
    UInt32 sizeBytes,
    UInt32& outPhysical,
    UInt32& outVirtual,
    UInt32& outSize
  ) {
    if (sizeBytes == 0) {
      return false;
    }

    if (sizeBytes > Arch::Memory::pageSize) {
      return false;
    }

    if (_dmaBufferPhysical == 0) {
      void* page = Arch::Memory::AllocatePageBelow(
        _dmaMaxPhysicalAddress,
        true,
        0x10000
      );

      if (!page) {
        return false;
      }

      _dmaBufferPhysical = reinterpret_cast<UInt32>(page);
      _dmaBufferBytes = Arch::Memory::pageSize;
    }

    UInt32 directory = Task::GetCurrentAddressSpace();

    if (directory == 0) {
      return false;
    }

    Memory::MapPageInAddressSpace(
      directory,
      _dmaBufferVirtualBase,
      _dmaBufferPhysical,
      true,
      true,
      false
    );

    outPhysical = _dmaBufferPhysical;
    outVirtual = _dmaBufferVirtualBase;
    outSize = _dmaBufferBytes;

    return true;
  }

  UInt32 BlockDevice::Register(BlockDevice::Device* device) {
    if (!device || _deviceCount >= _maxDevices) {
      return 0;
    }

    UInt32 id = _nextDeviceId++;

    device->info.id = id;
    device->portId = 0;
    _devices[_deviceCount++] = device;

    return id;
  }

  UInt32 BlockDevice::RegisterUser(const BlockDevice::Info& info) {
    if (_deviceCount >= _maxDevices) {
      return 0;
    }

    if (
      info.type == Type::Unknown
      || info.sectorSize == 0
      || info.sectorCount == 0
    ) {
      return 0;
    }

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      Device* device = _devices[i];

      if (!device) {
        continue;
      }

      if (
        device->info.type == info.type
        && device->info.deviceIndex == info.deviceIndex
      ) {
        return 0;
      }
    }

    Device* storage = nullptr;

    for (UInt32 i = 0; i < _maxDevices; ++i) {
      if (_deviceStorage[i].info.id == 0) {
        storage = &_deviceStorage[i];

        break;
      }
    }

    if (!storage) {
      return 0;
    }

    UInt32 id = _nextDeviceId++;

    storage->info = info;
    storage->info.id = id;
    storage->info.flags &= ~flagReady;
    storage->portId = 0;

    _devices[_deviceCount++] = storage;

    return id;
  }

  bool BlockDevice::Unregister(UInt32 deviceId) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_devices[i] && _devices[i]->info.id == deviceId) {
        _devices[i]->info.id = 0;
        _devices[i]->portId = 0;
        _devices[i] = _devices[_deviceCount - 1];
        _devices[_deviceCount - 1] = nullptr;
        _deviceCount--;

        return true;
      }
    }

    return false;
  }

  UInt32 BlockDevice::GetCount() {
    return _deviceCount;
  }

  bool BlockDevice::GetInfo(UInt32 deviceId, BlockDevice::Info& outInfo) {
    BlockDevice::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    outInfo = device->info;

    return true;
  }

  bool BlockDevice::UpdateInfo(UInt32 deviceId, const BlockDevice::Info& info) {
    BlockDevice::Device* device = Find(deviceId);

    if (!device) {
      return false;
    }

    if (info.id != deviceId || info.type != device->info.type) {
      return false;
    }

    if (device->portId == 0) {
      return false;
    }

    UInt32 ownerId = 0;

    if (!IPC::GetPortOwner(device->portId, ownerId)) {
      return false;
    }

    if (ownerId != Task::GetCurrentId()) {
      return false;
    }

    if (info.sectorSize == 0 || info.sectorCount == 0) {
      return false;
    }

    device->info.sectorSize = info.sectorSize;
    device->info.sectorCount = info.sectorCount;

    return true;
  }

  bool BlockDevice::Read(const BlockDevice::Request& request) {
    BlockDevice::Device* device = Find(request.deviceId);

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
      UInt32 sectorSize = device->info.sectorSize;
      UInt32 maxPerChunk = 0;

      if (sectorSize != 0) {
        maxPerChunk = messageDataBytes / sectorSize;
      }

      if (maxPerChunk == 0) {
        return false;
      }

      UInt32 remaining = request.count;
      UInt32 lba = request.lba;
      UInt8* buffer = reinterpret_cast<UInt8*>(request.buffer);

      while (remaining > 0) {
        UInt32 toRead = remaining < maxPerChunk ? remaining : maxPerChunk;
        Request chunk {};

        chunk.deviceId = request.deviceId;
        chunk.lba = lba;
        chunk.count = toRead;
        chunk.buffer = buffer;

        if (!SendRequest(*device, chunk, false)) {
          return false;
        }

        UInt32 bytes = toRead * sectorSize;

        remaining -= toRead;
        lba += toRead;
        buffer += bytes;
      }

      return true;
    }

    return false;
  }

  bool BlockDevice::Write(const BlockDevice::Request& request) {
    BlockDevice::Device* device = Find(request.deviceId);

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
      UInt32 sectorSize = device->info.sectorSize;
      UInt32 maxPerChunk = 0;

      if (sectorSize != 0) {
        maxPerChunk = messageDataBytes / sectorSize;
      }

      if (maxPerChunk == 0) {
        return false;
      }

      UInt32 remaining = request.count;
      UInt32 lba = request.lba;
      const UInt8* buffer
        = reinterpret_cast<const UInt8*>(request.buffer);

      while (remaining > 0) {
        UInt32 toWrite = remaining < maxPerChunk ? remaining : maxPerChunk;
        Request chunk {};

        chunk.deviceId = request.deviceId;
        chunk.lba = lba;
        chunk.count = toWrite;
        chunk.buffer = const_cast<UInt8*>(buffer);

        if (!SendRequest(*device, chunk, true)) {
          return false;
        }

        UInt32 bytes = toWrite * sectorSize;

        remaining -= toWrite;
        lba += toWrite;
        buffer += bytes;
      }

      return true;
    }

    return false;
  }

  BlockDevice::Device* BlockDevice::Find(UInt32 deviceId) {
    for (UInt32 i = 0; i < _deviceCount; ++i) {
      if (_devices[i] && _devices[i]->info.id == deviceId) {
        return _devices[i];
      }
    }

    return nullptr;
  }

  bool BlockDevice::ValidateRequest(
    const BlockDevice::Device& device,
    const BlockDevice::Request& request
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

  bool BlockDevice::Bind(UInt32 deviceId, UInt32 portId) {
    BlockDevice::Device* device = Find(deviceId);

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

  bool BlockDevice::SendRequest(
    BlockDevice::Device& device,
    const BlockDevice::Request& request,
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

    Message msg {};

    msg.op = write ? Operation::Write : Operation::Read;
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

    Message response {};

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

    if (response.op != Operation::Response || response.status != 0) {
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

  void BlockDevice::CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }
}
