/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Arch/IA32/Floppy.cpp
 * IA32 floppy controller interrupt handling.
 */

#include "Arch/IA32/BootInfo.hpp"
#include "Arch/IA32/Floppy.hpp"
#include "Arch/IA32/IO.hpp"
#include "Arch/IA32/PIC.hpp"
#include "Arch/IA32/Prelude.hpp"
#include "Devices/BlockDevice.hpp"
#include "Interrupts.hpp"
#include "Logger.hpp"
#include "Prelude.hpp"

namespace Quantum::System::Kernel::Arch::IA32 {
  using BlockDevice = Kernel::Devices::BlockDevice;
  using BootInfo = KernelIA32::BootInfo;
  using IO = KernelIA32::IO;
  using LogLevel = Kernel::Logger::Level;

  BlockDevice::Device Floppy::_devices[2] = {
    BlockDevice::Device{
      BlockDevice::Info{
        0,
        BlockDevice::Type::Floppy,
        512,
        _defaultSectorCount,
        BlockDevice::flagRemovable,
        _driveAIndex
      },
      0
    },
    BlockDevice::Device{
      BlockDevice::Info{
        0,
        BlockDevice::Type::Floppy,
        512,
        _defaultSectorCount,
        BlockDevice::flagRemovable,
        _driveBIndex
      },
      0
    }
  };

  UInt8 Floppy::ReadCMOSRegister(UInt8 index) {
    IO::Out8(
      _cmosAddressPort,
      static_cast<UInt8>(0x80 | (index & 0x7F))
    );

    return IO::In8(_cmosDataPort);
  }

  bool Floppy::TryGetSectorCount(
    UInt8 driveType,
    UInt32& sectorCount
  ) {
    switch (driveType) {
      case 0x1: {
        sectorCount = 40 * 2 * 9;

        return true;
      }

      case 0x2: {
        sectorCount = 80 * 2 * 15;

        return true;
      }

      case 0x3: {
        sectorCount = 80 * 2 * 9;

        return true;
      }

      case 0x4: {
        sectorCount = 80 * 2 * 18;

        return true;
      }

      case 0x5: {
        sectorCount = 80 * 2 * 36;

        return true;
      }

      default: {
        break;
      }
    }

    return false;
  }

  UInt8 Floppy::GetDriveType(UInt8 driveTypes, UInt8 driveIndex) {
    if (driveIndex == _driveAIndex) {
      return static_cast<UInt8>((driveTypes >> 4) & 0x0F);
    }

    if (driveIndex == _driveBIndex) {
      return static_cast<UInt8>(driveTypes & 0x0F);
    }

    return 0;
  }

  bool Floppy::DetectDrive(
    UInt8 driveTypes,
    UInt8 driveIndex,
    UInt8& driveType,
    UInt32& sectorCount
  ) {
    driveType = GetDriveType(driveTypes, driveIndex);

    if (driveType == 0) {
      return false;
    }

    return TryGetSectorCount(driveType, sectorCount);
  }

  bool Floppy::GetBootDrive(UInt8& bootDrive) {
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

  Interrupts::Context* Floppy::IRQHandler(
    Interrupts::Context& context
  ) {
    BlockDevice::NotifyIRQ(BlockDevice::Type::Floppy);

    return &context;
  }

  void Floppy::Initialize() {
    Interrupts::RegisterHandler(38, IRQHandler); // IRQ6 vector
    PIC::Unmask(6);

    UInt8 driveTypes = ReadCMOSRegister(_cmosDriveTypeRegister);
    bool registered = false;

    for (UInt8 driveIndex = 0; driveIndex < 2; ++driveIndex) {
      UInt8 driveType = 0;
      UInt32 sectorCount = 0;

      if (!DetectDrive(driveTypes, driveIndex, driveType, sectorCount)) {
        continue;
      }

      BlockDevice::Device& device = _devices[driveIndex];

      device.info.type = BlockDevice::Type::Floppy;
      device.info.sectorSize = 512;
      device.info.sectorCount = sectorCount;
      device.info.flags = BlockDevice::flagRemovable;
      device.info.deviceIndex = driveIndex;
      device.portId = 0;

      UInt32 id = BlockDevice::Register(&device);
      char driveLetter = driveIndex == _driveAIndex ? 'A' : 'B';

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
      UInt8 driveIndex = bootDrive == 0x01
        ? _driveBIndex
        : _driveAIndex;
      BlockDevice::Device& device = _devices[driveIndex];

      device.info.type = BlockDevice::Type::Floppy;
      device.info.sectorSize = 512;
      device.info.sectorCount = _defaultSectorCount;
      device.info.flags = BlockDevice::flagRemovable;
      device.info.deviceIndex = driveIndex;
      device.portId = 0;

      UInt32 id = BlockDevice::Register(&device);
      char driveLetter = driveIndex == _driveAIndex ? 'A' : 'B';

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
}
