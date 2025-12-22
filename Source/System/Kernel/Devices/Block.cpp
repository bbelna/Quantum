/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Devices/Block.cpp
 * Kernel block device registry and interface.
 */

#include <Arch/IA32/BootInfo.hpp>
#include <Arch/IA32/IO.hpp>
#include <Arch/IA32/Memory.hpp>
#include <Devices/Block.hpp>
#include <IPC.hpp>
#include <Logger.hpp>
#include <Memory.hpp>
#include <Task.hpp>

namespace Quantum::System::Kernel::Devices {
  using LogLevel = Logger::Level;
  using ArchMemory = ::Quantum::System::Kernel::Arch::IA32::Memory;
  using IPC = ::Quantum::System::Kernel::IPC;
  using Memory = ::Quantum::System::Kernel::Memory;
  using Task = ::Quantum::System::Kernel::Task;
  using BootInfo = ::Quantum::System::Kernel::Arch::IA32::BootInfo;
  using IO = ::Quantum::System::Kernel::Arch::IA32::IO;

  UInt8 Block::ReadCMOSRegister(UInt8 index) {
    IO::Out8(
      _cmosAddressPort,
      static_cast<UInt8>(0x80 | (index & 0x7F))
    );
    return IO::In8(_cmosDataPort);
  }

  bool Block::TryGetFloppySectorCount(UInt8 driveType, UInt32& sectorCount) {
    switch (driveType) {
      case 0x1:
        sectorCount = 40 * 2 * 9;
        return true;
      case 0x2:
        sectorCount = 80 * 2 * 15;
        return true;
      case 0x3:
        sectorCount = 80 * 2 * 9;
        return true;
      case 0x4:
        sectorCount = 80 * 2 * 18;
        return true;
      case 0x5:
        sectorCount = 80 * 2 * 36;
        return true;
      default:
        break;
    }

    return false;
  }

  UInt8 Block::GetFloppyDriveType(UInt8 driveTypes, UInt8 driveIndex) {
    if (driveIndex == _floppyDriveAIndex) {
      return static_cast<UInt8>((driveTypes >> 4) & 0x0F);
    }

    if (driveIndex == _floppyDriveBIndex) {
      return static_cast<UInt8>(driveTypes & 0x0F);
    }

    return 0;
  }

  bool Block::DetectFloppyDrive(
    UInt8 driveTypes,
    UInt8 driveIndex,
    UInt8& driveType,
    UInt32& sectorCount
  ) {
    driveType = GetFloppyDriveType(driveTypes, driveIndex);

    if (driveType == 0) {
      return false;
    }

    return TryGetFloppySectorCount(driveType, sectorCount);
  }

  bool Block::GetBootDrive(UInt8& bootDrive) {
    const BootInfo::View* bootInfo = BootInfo::Get();

    if (!bootInfo) {
      return false;
    }

    UInt32 reserved = bootInfo->reserved;

    if ((reserved & 0xFFFF0000u) != _bootDriveMagic) {
      return false;
    }

    bootDrive = static_cast<UInt8>(reserved & 0xFF);
    return true;
  }

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

  Block::Device Block::_floppyDevices[Block::_maxFloppyDevices] = {
    Block::Device{
      Block::Info{
        0,
        Block::Type::Floppy,
        512,
        2880,
        Block::flagRemovable,
        _floppyDriveAIndex
      },
      0,
      &FloppyStubRead,
      &FloppyStubWrite
    },
    Block::Device{
      Block::Info{
        0,
        Block::Type::Floppy,
        512,
        2880,
        Block::flagRemovable,
        _floppyDriveBIndex
      },
      0,
      &FloppyStubRead,
      &FloppyStubWrite
    }
  };

  UInt32 Block::_dmaBufferPhysical = 0;
  UInt32 Block::_dmaBufferBytes = 0;

  void Block::Initialize() {
    _deviceCount = 0;
    _nextDeviceId = 1;

    UInt8 driveTypes = ReadCMOSRegister(_cmosDriveTypeRegister);
    bool registered = false;

    for (UInt8 driveIndex = 0; driveIndex < _maxFloppyDevices; ++driveIndex) {
      UInt8 driveType = 0;
      UInt32 sectorCount = 0;

      if (!DetectFloppyDrive(driveTypes, driveIndex, driveType, sectorCount)) {
        continue;
      }

      Block::Device& device = _floppyDevices[driveIndex];

      device.info.type = Block::Type::Floppy;
      device.info.sectorSize = 512;
      device.info.sectorCount = sectorCount;
      device.info.flags = Block::flagRemovable;
      device.info.deviceIndex = driveIndex;
      device.portId = 0;

      UInt32 id = Register(&device);
      char driveLetter = driveIndex == _floppyDriveAIndex ? 'A' : 'B';

      if (id == 0) {
        Logger::WriteFormatted(
          LogLevel::Warning,
          "BlockDevices: failed to register floppy %c",
          driveLetter
        );
      } else {
        Logger::WriteFormatted(
          LogLevel::Info,
          "BlockDevices: registered floppy %c id=%u type=0x%x",
          driveLetter,
          id,
          driveType
        );
        registered = true;
      }
    }

    if (registered) {
      return;
    }

    UInt8 bootDrive = 0xFF;

    if (GetBootDrive(bootDrive) && bootDrive < 0x80) {
      UInt8 driveIndex
        = bootDrive == 0x01 ? _floppyDriveBIndex : _floppyDriveAIndex;
      Block::Device& device = _floppyDevices[driveIndex];

      device.info.type = Block::Type::Floppy;
      device.info.sectorSize = 512;
      device.info.sectorCount = _defaultFloppySectorCount;
      device.info.flags = Block::flagRemovable;
      device.info.deviceIndex = driveIndex;
      device.portId = 0;

      UInt32 id = Register(&device);
      char driveLetter = driveIndex == _floppyDriveAIndex ? 'A' : 'B';

      if (id == 0) {
        Logger::Write(
          LogLevel::Warning,
          "BlockDevices: failed to register fallback"
        );
      } else {
        Logger::WriteFormatted(
          LogLevel::Debug,
          "BlockDevices: CMOS empty; using boot drive %c",
          driveLetter
        );
      }
    } else {
      Logger::Write(LogLevel::Debug, "BlockDevices: no floppy detected");
    }
  }

  void Block::HandleFloppyIRQ() {
    Message msg{};

    msg.op = Operation::Response;
    msg.lba = 0;
    msg.count = 0;
    msg.replyPortId = 0;
    msg.status = 0;
    msg.dataLength = 0;

    UInt32 senderId = Task::GetCurrentId();

    for (UInt32 i = 0; i < _deviceCount; ++i) {
      Device* device = _devices[i];

      if (!device || device->portId == 0) {
        continue;
      }

      if (device->info.type != Type::Floppy) {
        continue;
      }

      msg.deviceId = device->info.id;

      IPC::Send(device->portId, senderId, &msg, messageHeaderBytes);
    }
  }

  bool Block::AllocateDMABuffer(
    UInt32 sizeBytes,
    UInt32& outPhysical,
    UInt32& outVirtual,
    UInt32& outSize
  ) {
    if (sizeBytes == 0) {
      return false;
    }

    if (sizeBytes > ArchMemory::pageSize) {
      return false;
    }

    if (_dmaBufferPhysical == 0) {
      void* page = ArchMemory::AllocatePageBelow(
        _dmaMaxPhysicalAddress,
        true
      );

      if (!page) {
        return false;
      }

      _dmaBufferPhysical = reinterpret_cast<UInt32>(page);
      _dmaBufferBytes = ArchMemory::pageSize;
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

  void Block::CopyBytes(void* dest, const void* src, UInt32 length) {
    auto* d = reinterpret_cast<UInt8*>(dest);
    auto* s = reinterpret_cast<const UInt8*>(src);

    for (UInt32 i = 0; i < length; ++i) {
      d[i] = s[i];
    }
  }

}
