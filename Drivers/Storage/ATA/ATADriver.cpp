/**
 * @file Drivers/Storage/ATA/ATADriver.cpp
 * @brief Implements @ref @QDrvs::Storage::ATA::ATADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ATADriver.hpp"

namespace Quantum::Drivers::Storage::ATA {
  ATADriver::ATADriver(KernelClient& kernel) : _kernel(kernel) {}

  bool ATADriver::Probe() {
    // detect hypervisor via the CPU platform driver so we can skip
    // unnecessary delays on emulated hardware
    {
      namespace CPUHAL = ::Quantum::HAL::CPU;

      auto devices = _kernel.GetDevices();

      for (UInt32 i = 0; i < devices.GetCount(); i++) {
        if (devices[i].ID == 6) {
          CPUHAL::CPUHypervisorInfo info = {};

          _hypervisor = _kernel.InvokeDriver(
            devices[i].ID,
            static_cast<UInt32>(CPUHAL::CPUDriverOperation::GetHypervisorInfo),
            &info
          ) != 0;

          if (_hypervisor) {
            _kernel.WriteLog(
              Core::LogLevel::Info,
              "ATA: hypervisor detected, reducing delays"
            );
          }

          break;
        }
      }
    }

    _probeBMIDE();

    _probeChannel(0); // primary: indices 0 (master) and 1 (slave)
    _probeChannel(2); // secondary: indices 2 (master) and 3 (slave)

    // claim IRQs for channels with detected drives, then re-enable
    // interrupts on the controller (clear nIEN)
    if (_drives[0].Present || _drives[1].Present) {
      _primaryIRQHandle = _kernel.ClaimInterrupt(14);
      _kernel.PortOut8(PrimaryCtrl, 0x00);
    }

    if (_drives[2].Present || _drives[3].Present) {
      _secondaryIRQHandle = _kernel.ClaimInterrupt(15);
      _kernel.PortOut8(SecondaryCtrl, 0x00);
    }

    for (UInt8 index = 0; index < 4; index++) {
      if (_drives[index].Present) {
        _kernel.WriteLog(
          Core::LogLevel::Info,
          "ATA[%u]: %s, %u sectors, model \"%s\"",
          index,
          _drives[index].Type == StorageABI::StorageDeviceType::HardDisk
            ? "HardDisk" : "Optical",
          static_cast<UInt32>(_drives[index].SectorCount),
          _drives[index].Model
        );
      }
    }

    for (UInt8 index = 0; index < 4; index++) {
      if (_drives[index].Present) return true;
    }

    _kernel.WriteLog(Core::LogLevel::Warning, "ATA: no drives detected");

    return false;
  }

  void ATADriver::_probeChannel(UInt8 base) {
    UInt16 ioBase = _base(base);
    UInt16 ctrlPort = _ctrl(base);

    // Suppress IRQs during probing (nIEN = bit 1 of Device Control).
    // IDENTIFY generates an interrupt on completion; without a handler
    // registered the IRQ would stay pending at the PIC and satisfy the
    // first WaitForInterrupt prematurely once the channel is live.
    _kernel.PortOut8(ctrlPort, 0x02);

    for (UInt8 driveIndex = 0; driveIndex < 2; driveIndex++) {
      UInt8 localIndex = base + driveIndex;
      UInt8 driveSelect = 0xA0 | (driveIndex << 4);

      // select drive
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::DriveHead), driveSelect);

      // short delay (400 ns) via alternate status reads; instant on
      // emulated hardware
      if (!_hypervisor) {
        for (UInt8 delay = 0; delay < 4; delay++) {
          _kernel.PortIn8(static_cast<UInt16>(_ctrl(localIndex)));
        }
      }

      // check if a drive is present: non-floating sector count or LBA
      // registers
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::SectorCount), 0xAB);
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::LBALow), 0xCD);

      if (_kernel.PortIn8(ATAPort(ioBase, ATARegister::SectorCount)) != 0xAB) {
        continue;
      }

      if (_kernel.PortIn8(ATAPort(ioBase, ATARegister::LBALow)) != 0xCD) {
        continue;
      }

      // issue IDENTIFY to determine drive type and capacity
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::SectorCount), 0);
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::LBALow), 0);
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::LBAMid), 0);
      _kernel.PortOut8(ATAPort(ioBase, ATARegister::LBAHigh), 0);
      _kernel.PortOut8(
        ATAPort(ioBase, ATARegister::Status),
        static_cast<UInt8>(ATACommand::Identify)
      );

      if (!_waitBSY(ioBase)) continue;

      UInt8 status = _kernel.PortIn8(ATAPort(ioBase, ATARegister::Status));

      if (status == 0) continue; // no drive

      // check if it's ATAPI (LBAMid/High non-zero after IDENTIFY)
      UInt8 lbaMid = _kernel.PortIn8(ATAPort(ioBase, ATARegister::LBAMid));
      UInt8 lbaHigh = _kernel.PortIn8(ATAPort(ioBase, ATARegister::LBAHigh));

      bool isAtapi = (lbaMid == 0x14 && lbaHigh == 0xEB);

      if (isAtapi) {
        // issue IDENTIFY PACKET DEVICE
        _kernel.PortOut8(
          ATAPort(ioBase, ATARegister::Status),
          static_cast<UInt8>(ATACommand::IdentifyPacket)
        );

        if (!_waitBSY(ioBase)) continue;

        if (!_waitDRQ(ioBase)) continue;
      } else if (!_waitDRQ(ioBase)) {
        continue;
      }

      // read 256 words of IDENTIFY data
      UInt16 identifyBuffer[256];

      for (UInt16 word = 0; word < 256; word++) {
        identifyBuffer[word] = _kernel.PortIn16(
          ATAPort(ioBase, ATARegister::Data)
        );
      }

      DriveInfo& info = _drives[localIndex];

      info.Present = true;
      info.Type = isAtapi
        ? StorageABI::StorageDeviceType::OpticalDisk
        : StorageABI::StorageDeviceType::HardDisk;

      // sector count: prefer 28-bit LBA (words 60-61), fall back to
      // CHS geometry (words 1, 3, 6) for drives that don't report it
      if (!isAtapi) {
        UInt64 lbaSectors =
          static_cast<UInt64>(identifyBuffer[60]) |
          (static_cast<UInt64>(identifyBuffer[61]) << 16);

        if (lbaSectors != 0) {
          info.SectorCount = lbaSectors;
        } else {
          UInt64 cylinders = identifyBuffer[1];
          UInt64 heads     = identifyBuffer[3];
          UInt64 sectorsPerTrack = identifyBuffer[6];

          info.SectorCount = cylinders * heads * sectorsPerTrack;
        }
      }

      // model string at bytes 54-93 (words 27-46), byte-swapped
      UInt32 modelIndex = 0;

      for (UInt16 word = 27; word <= 46 && modelIndex < 40; word++) {
        info.Model[modelIndex++] = static_cast<char>(
          identifyBuffer[word] >> 8
        );
        info.Model[modelIndex++] = static_cast<char>(
          identifyBuffer[word] & 0xFF
        );
      }

      // trim trailing spaces
      while (modelIndex > 0 && info.Model[modelIndex - 1] == ' ') {
        modelIndex--;
      }

      info.Model[modelIndex] = '\0';
    }
  }

  const ATADriver::DriveInfo& ATADriver::GetDriveInfo(
    UInt8 localIndex
  ) const {
    return _drives[localIndex < 4 ? localIndex : 0];
  }

  StorageABI::StorageOperationErrorCode ATADriver::Read(
    UInt8 localIndex,
    UInt64 lba,
    UInt32 count,
    void* buffer
  ) {
    if (localIndex >= 4 || !_drives[localIndex].Present) {
      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: Read invalid device %u (present=%u)",
        localIndex,
        localIndex < 4 ? _drives[localIndex].Present : 0
      );

      return StorageABI::StorageOperationErrorCode::InvalidDevice;
    }

    // use DMA for ATA hard disks when BMIDE is available
    if (
      _dmaAvailable &&
      _drives[localIndex].Type == StorageABI::StorageDeviceType::HardDisk
    ) {
      UInt8* destination = static_cast<UInt8*>(buffer);
      UInt32 remaining = count;
      UInt64 currentLBA = lba;

      while (remaining > 0) {
        UInt32 batch = remaining;

        if (batch > DMAMaxSectors) batch = DMAMaxSectors;

        StorageABI::StorageOperationErrorCode error = _readDMA(
          localIndex, currentLBA, batch, destination
        );

        if (error != StorageABI::StorageOperationErrorCode::None) return error;

        destination += batch * 512;
        currentLBA += batch;
        remaining -= batch;
      }

      return StorageABI::StorageOperationErrorCode::None;
    }

    // PIO fallback
    UInt8* destination = static_cast<UInt8*>(buffer);

    for (UInt32 sector = 0; sector < count; sector++) {
      UInt32 sectorAddress = static_cast<UInt32>(lba + sector);

      if (!_setup28(localIndex, sectorAddress, ATACommand::ReadSectors)) {
        _kernel.WriteLog(
          Core::LogLevel::Error,
          "ATA: Read setup28 failed, dev %u LBA %u",
          localIndex, sectorAddress
        );

        return StorageABI::StorageOperationErrorCode::ReadError;
      }

      UInt16 ioBase = _base(localIndex);
      UInt16* wordBuffer = reinterpret_cast<UInt16*>(destination);

      for (UInt16 word = 0; word < 256; word++) {
        wordBuffer[word] = _kernel.PortIn16(
          ATAPort(ioBase, ATARegister::Data)
        );
      }

      destination += 512;
    }

    return StorageABI::StorageOperationErrorCode::None;
  }

  StorageABI::StorageOperationErrorCode ATADriver::Write(
    UInt8 localIndex,
    UInt64 lba,
    UInt32 count,
    const void* buffer
  ) {
    if (localIndex >= 4 || !_drives[localIndex].Present) {
      return StorageABI::StorageOperationErrorCode::InvalidDevice;
    }

    // use DMA for ATA hard disks when BMIDE is available
    if (
      _dmaAvailable &&
      _drives[localIndex].Type == StorageABI::StorageDeviceType::HardDisk
    ) {
      const UInt8* source = static_cast<const UInt8*>(buffer);
      UInt32 remaining = count;
      UInt64 currentLBA = lba;

      while (remaining > 0) {
        UInt32 batch = remaining;

        if (batch > DMAMaxSectors) batch = DMAMaxSectors;

        StorageABI::StorageOperationErrorCode error = _writeDMA(
          localIndex, currentLBA, batch, source
        );

        if (error != StorageABI::StorageOperationErrorCode::None) return error;

        source += batch * 512;
        currentLBA += batch;
        remaining -= batch;
      }

      return StorageABI::StorageOperationErrorCode::None;
    }

    // PIO fallback
    const UInt8* source = static_cast<const UInt8*>(buffer);
    Kernel::Resources::ResourceID irqResource = _irqHandle(localIndex);

    for (UInt32 sector = 0; sector < count; sector++) {
      UInt32 sectorAddress = static_cast<UInt32>(lba + sector);

      if (!_setup28(localIndex, sectorAddress, ATACommand::WriteSectors)) {
        return StorageABI::StorageOperationErrorCode::WriteError;
      }

      UInt16 ioBase = _base(localIndex);
      const UInt16* wordBuffer = reinterpret_cast<const UInt16*>(source);

      for (UInt16 word = 0; word < 256; word++) {
        _kernel.PortOut16(
          ATAPort(ioBase, ATARegister::Data), wordBuffer[word]
        );
      }

      source += 512;

      // wait for the drive to finish writing -- the IRQ fires when the
      // write completes, allowing the CPU to sleep instead of polling
      if (irqResource != static_cast<Kernel::Resources::ResourceID>(-1)) {
        _kernel.WaitForInterrupt(irqResource);

        UInt8 status = _kernel.PortIn8(
          ATAPort(ioBase, ATARegister::Status)
        );

        if (status & static_cast<UInt8>(ATAStatus::Error)) {
          return StorageABI::StorageOperationErrorCode::WriteError;
        }
      } else {
        if (!_waitBSY(ioBase)) {
          return StorageABI::StorageOperationErrorCode::WriteError;
        }
      }
    }

    return StorageABI::StorageOperationErrorCode::None;
  }

  StorageABI::StorageOperationErrorCode ATADriver::Flush(UInt8 localIndex) {
    if (localIndex >= 4 || !_drives[localIndex].Present) {
      return StorageABI::StorageOperationErrorCode::InvalidDevice;
    }

    _select(localIndex);

    UInt16 ioBase = _base(localIndex);

    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::Status),
      static_cast<UInt8>(ATACommand::FlushCache)
    );

    Kernel::Resources::ResourceID irqResource = _irqHandle(localIndex);

    if (irqResource != static_cast<Kernel::Resources::ResourceID>(-1)) {
      _kernel.WaitForInterrupt(irqResource);

      UInt8 status = _kernel.PortIn8(
        ATAPort(ioBase, ATARegister::Status)
      );

      return (status & static_cast<UInt8>(ATAStatus::Error))
        ? StorageABI::StorageOperationErrorCode::WriteError
        : StorageABI::StorageOperationErrorCode::None;
    }

    return _waitBSY(ioBase)
      ? StorageABI::StorageOperationErrorCode::None
      : StorageABI::StorageOperationErrorCode::Timeout;
  }

  bool ATADriver::_waitBSY(UInt16 base) {
    UInt32 timeout = _hypervisor ? BsyTimeoutHypervisor : BsyTimeout;

    for (UInt32 iteration = 0; iteration < timeout; iteration++) {
      UInt8 status = _kernel.PortIn8(ATAPort(base, ATARegister::Status));

      if (!(status & static_cast<UInt8>(ATAStatus::Busy))) return true;
    }

    return false;
  }

  bool ATADriver::_waitDRQ(UInt16 base) {
    UInt32 timeout = _hypervisor ? DrqTimeoutHypervisor : DrqTimeout;

    for (UInt32 iteration = 0; iteration < timeout; iteration++) {
      UInt8 status = _kernel.PortIn8(ATAPort(base, ATARegister::Status));

      if (status & static_cast<UInt8>(ATAStatus::Error)) return false;
      if (status & static_cast<UInt8>(ATAStatus::DataRequest)) return true;
    }

    return false;
  }

  Kernel::Resources::ResourceID ATADriver::_irqHandle(
    UInt8 localIndex
  ) const {
    return (localIndex < 2) ? _primaryIRQHandle : _secondaryIRQHandle;
  }

  void ATADriver::_select(UInt8 localIndex) {
    UInt16 ioBase = _base(localIndex);
    UInt8 driveNumber = _driveOnChannel(localIndex);

    // LBA mode select; bits 5,7 are always 1
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::DriveHead),
      static_cast<UInt8>(0xE0 | (driveNumber << 4))
    );

    // 400 ns delay; instant on emulated hardware
    if (!_hypervisor) {
      for (UInt8 delay = 0; delay < 4; delay++) {
        _kernel.PortIn8(static_cast<UInt16>(_ctrl(localIndex)));
      }
    }

    _waitBSY(ioBase);
  }

  void ATADriver::_probeBMIDE() {
    // find the PCI IDE controller (class 0x01, subclass 0x01) via the
    // kernel PCI driver
    auto devices = _kernel.GetDevices();
    UInt32 pciDeviceID = 0;

    for (UInt32 i = 0; i < devices.GetCount(); i++) {
      if (devices[i].ID == 11) {
        pciDeviceID = devices[i].ID;

        break;
      }
    }

    if (pciDeviceID == 0) {
      _kernel.WriteLog(
        Core::LogLevel::Info,
        "ATA: PCI driver not found, DMA unavailable"
      );

      return;
    }

    PCIDriverClient pci;

    pci.Initialize(pciDeviceID);

    HAL::PCI::PCIDeviceInfo ide = {};

    if (!pci.FindDeviceByClass(0x01, 0x01, ide)) {
      _kernel.WriteLog(
        Core::LogLevel::Info,
        "ATA: No PCI IDE controller found, DMA unavailable"
      );

      return;
    }

    // read BAR4 — the Bus Master IDE register base
    UInt32 bar4 = pci.ReadBAR(ide.Bus, ide.Slot, ide.Function, 4);

    if (bar4 == 0) {
      _kernel.WriteLog(
        Core::LogLevel::Info,
        "ATA: IDE controller BAR4 is zero, DMA unavailable"
      );

      return;
    }

    // BAR4 is an I/O BAR; mask off the lower 2 flag bits
    UInt16 bmideBase = static_cast<UInt16>(bar4 & 0xFFFC);

    // enable bus mastering on the IDE controller
    pci.EnableBusMastering(ide.Bus, ide.Slot, ide.Function);

    _bmidePrimaryBase = bmideBase;
    _bmideSecondaryBase = static_cast<UInt16>(bmideBase + 0x08);

    // allocate the PRD table (one entry is 8 bytes; we only need one entry
    // since we use a contiguous bounce buffer, but allocate a full page for
    // alignment)
    UIntPtr prdVirtual = _kernel.AllocateDMA(
      sizeof(PRDEntry) * 2, 0xFFFFFFFF
    );

    if (prdVirtual == 0) {
      _kernel.WriteLog(
        Core::LogLevel::Warning,
        "ATA: Failed to allocate PRD table"
      );

      return;
    }

    _prdTable = reinterpret_cast<PRDEntry*>(prdVirtual);
    _prdTablePhysical = _kernel.VirtualToPhysical(prdVirtual);

    // allocate the DMA bounce buffer (64 KB)
    UIntPtr bufferVirtual = _kernel.AllocateDMA(
      DMABufferSize, 0xFFFFFFFF
    );

    if (bufferVirtual == 0) {
      _kernel.WriteLog(
        Core::LogLevel::Warning,
        "ATA: Failed to allocate DMA buffer"
      );

      FreeBlock(prdVirtual);

      _prdTable = nullptr;
      _prdTablePhysical = 0;

      return;
    }

    _dmaBuffer = reinterpret_cast<void*>(bufferVirtual);
    _dmaBufferPhysical = _kernel.VirtualToPhysical(bufferVirtual);

    _dmaAvailable = true;

    _kernel.WriteLog(
      Core::LogLevel::Info,
      "ATA: BMIDE at 0x%x (PCI %u:%u.%u), DMA enabled",
      bmideBase, ide.Bus, ide.Slot, ide.Function
    );
  }

  void ATADriver::_setupPRD(UInt32 byteCount) {
    _prdTable[0].PhysicalAddress = _dmaBufferPhysical;
    _prdTable[0].ByteCount = byteCount | (1u << 31); // EOT flag
  }

  StorageABI::StorageOperationErrorCode ATADriver::_readDMA(
    UInt8 localIndex,
    UInt64 lba,
    UInt32 count,
    void* buffer
  ) {
    UInt16 bmBase = _bmideBase(localIndex);
    UInt16 ioBase = _base(localIndex);
    UInt8 driveNumber = _driveOnChannel(localIndex);
    UInt32 byteCount = count * 512;
    Kernel::Resources::ResourceID irqResource = _irqHandle(localIndex);

    // program the PRD table
    _setupPRD(byteCount);

    // stop the DMA engine and clear error/interrupt status
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
      0x00
    );
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Status)),
      0x06 // clear error (bit 1) and interrupt (bit 2) by writing 1
    );

    // load PRD table address
    _kernel.PortOut32(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::PRDTableAddress)),
      _prdTablePhysical
    );

    // select drive and set up LBA registers for multi-sector transfer
    _select(localIndex);

    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::SectorCount),
      static_cast<UInt8>(count)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBALow),
      static_cast<UInt8>(lba & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBAMid),
      static_cast<UInt8>((lba >> 8) & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBAHigh),
      static_cast<UInt8>((lba >> 16) & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::DriveHead),
      static_cast<UInt8>(0xE0 | (driveNumber << 4) | ((lba >> 24) & 0x0F))
    );

    // issue READ DMA command
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::Status),
      static_cast<UInt8>(ATACommand::ReadDMA)
    );

    // start the DMA engine in read direction (bit 3 = 1 for read, bit 0 = start)
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
      0x09
    );

    // wait for the transfer to complete via IRQ
    if (irqResource != static_cast<Kernel::Resources::ResourceID>(-1)) {
      _kernel.WaitForInterrupt(irqResource);
    } else {
      if (!_waitBSY(ioBase)) {
        // stop DMA engine
        _kernel.PortOut8(
          static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
          0x00
        );

        _kernel.WriteLog(
          Core::LogLevel::Error,
          "ATA: DMA read BSY timeout, LBA %u",
          static_cast<UInt32>(lba)
        );

        return StorageABI::StorageOperationErrorCode::Timeout;
      }
    }

    // stop the DMA engine
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
      0x00
    );

    // check BM status for errors
    UInt8 bmStatus = _kernel.PortIn8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Status))
    );

    // clear interrupt and error bits
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Status)),
      0x06
    );

    if (bmStatus & 0x02) {
      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: DMA read error, bmStatus 0x%x LBA %u",
        bmStatus, static_cast<UInt32>(lba)
      );

      return StorageABI::StorageOperationErrorCode::ReadError;
    }

    // check ATA status
    UInt8 ataStatus = _kernel.PortIn8(ATAPort(ioBase, ATARegister::Status));

    if (ataStatus & static_cast<UInt8>(ATAStatus::Error)) {
      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: DMA read ATA error, status 0x%x LBA %u",
        ataStatus, static_cast<UInt32>(lba)
      );

      return StorageABI::StorageOperationErrorCode::ReadError;
    }

    // copy from bounce buffer to caller's buffer
    const UInt8* source = static_cast<const UInt8*>(_dmaBuffer);
    UInt8* destination = static_cast<UInt8*>(buffer);

    for (UInt32 i = 0; i < byteCount; i++) {
      destination[i] = source[i];
    }

    return StorageABI::StorageOperationErrorCode::None;
  }

  StorageABI::StorageOperationErrorCode ATADriver::_writeDMA(
    UInt8 localIndex,
    UInt64 lba,
    UInt32 count,
    const void* buffer
  ) {
    UInt16 bmBase = _bmideBase(localIndex);
    UInt16 ioBase = _base(localIndex);
    UInt8 driveNumber = _driveOnChannel(localIndex);
    UInt32 byteCount = count * 512;
    Kernel::Resources::ResourceID irqResource = _irqHandle(localIndex);

    // copy data to bounce buffer
    const UInt8* source = static_cast<const UInt8*>(buffer);
    UInt8* destination = static_cast<UInt8*>(_dmaBuffer);

    for (UInt32 i = 0; i < byteCount; i++) {
      destination[i] = source[i];
    }

    // program the PRD table
    _setupPRD(byteCount);

    // stop the DMA engine and clear error/interrupt status
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
      0x00
    );
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Status)),
      0x06
    );

    // load PRD table address
    _kernel.PortOut32(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::PRDTableAddress)),
      _prdTablePhysical
    );

    // select drive and set up LBA registers
    _select(localIndex);

    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::SectorCount),
      static_cast<UInt8>(count)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBALow),
      static_cast<UInt8>(lba & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBAMid),
      static_cast<UInt8>((lba >> 8) & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBAHigh),
      static_cast<UInt8>((lba >> 16) & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::DriveHead),
      static_cast<UInt8>(0xE0 | (driveNumber << 4) | ((lba >> 24) & 0x0F))
    );

    // issue WRITE DMA command
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::Status),
      static_cast<UInt8>(ATACommand::WriteDMA)
    );

    // start the DMA engine in write direction (bit 3 = 0 for write, bit 0 = start)
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
      0x01
    );

    // wait for the transfer to complete via IRQ
    if (irqResource != static_cast<Kernel::Resources::ResourceID>(-1)) {
      _kernel.WaitForInterrupt(irqResource);
    } else {
      if (!_waitBSY(ioBase)) {
        _kernel.PortOut8(
          static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
          0x00
        );

        _kernel.WriteLog(
          Core::LogLevel::Error,
          "ATA: DMA write BSY timeout, LBA %u",
          static_cast<UInt32>(lba)
        );

        return StorageABI::StorageOperationErrorCode::Timeout;
      }
    }

    // stop the DMA engine
    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Command)),
      0x00
    );

    // check BM status
    UInt8 bmStatus = _kernel.PortIn8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Status))
    );

    _kernel.PortOut8(
      static_cast<UInt16>(bmBase + static_cast<UInt8>(BMIDERegister::Status)),
      0x06
    );

    if (bmStatus & 0x02) {
      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: DMA write error, bmStatus 0x%x LBA %u",
        bmStatus, static_cast<UInt32>(lba)
      );

      return StorageABI::StorageOperationErrorCode::WriteError;
    }

    UInt8 ataStatus = _kernel.PortIn8(ATAPort(ioBase, ATARegister::Status));

    if (ataStatus & static_cast<UInt8>(ATAStatus::Error)) {
      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: DMA write ATA error, status 0x%x LBA %u",
        ataStatus, static_cast<UInt32>(lba)
      );

      return StorageABI::StorageOperationErrorCode::WriteError;
    }

    return StorageABI::StorageOperationErrorCode::None;
  }

  bool ATADriver::_setup28(
    UInt8 localIndex,
    UInt32 lba,
    ATACommand command
  ) {
    UInt16 ioBase = _base(localIndex);
    UInt8 driveNumber = _driveOnChannel(localIndex);

    _select(localIndex);

    // 28-bit LBA setup
    _kernel.PortOut8(ATAPort(ioBase, ATARegister::SectorCount), 1);
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBALow),
      static_cast<UInt8>(lba & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBAMid),
      static_cast<UInt8>((lba >> 8) & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::LBAHigh),
      static_cast<UInt8>((lba >> 16) & 0xFF)
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::DriveHead),
      static_cast<UInt8>(0xE0 | (driveNumber << 4) | ((lba >> 24) & 0x0F))
    );
    _kernel.PortOut8(
      ATAPort(ioBase, ATARegister::Status),
      static_cast<UInt8>(command)
    );

    // for read commands, the IRQ fires when data is ready (BSY clear,
    // DRQ set); use WaitForInterrupt to sleep instead of polling
    // for write commands, DRQ is set nearly immediately (no IRQ) so
    // polling is fine and very fast
    Kernel::Resources::ResourceID irqResource = _irqHandle(localIndex);

    if (
      command == ATACommand::ReadSectors &&
      irqResource != static_cast<Kernel::Resources::ResourceID>(-1)
    ) {
      _kernel.WaitForInterrupt(irqResource);

      UInt8 status = _kernel.PortIn8(
        ATAPort(ioBase, ATARegister::Status)
      );

      if (status & static_cast<UInt8>(ATAStatus::Error)) {
        UInt8 error = _kernel.PortIn8(
          ATAPort(ioBase, ATARegister::Error)
        );

        _kernel.WriteLog(
          Core::LogLevel::Error,
          "ATA: setup28 IRQ path error, status 0x%x error 0x%x LBA %u",
          status, error, lba
        );

        return false;
      }

      if (!(status & static_cast<UInt8>(ATAStatus::DataRequest))) {
        _kernel.WriteLog(
          Core::LogLevel::Error,
          "ATA: setup28 IRQ path no DRQ, status 0x%x LBA %u",
          status, lba
        );

        return false;
      }

      return true;
    }

    if (!_waitBSY(ioBase)) {
      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: setup28 poll BSY timeout, LBA %u",
        lba
      );

      return false;
    }

    if (!_waitDRQ(ioBase)) {
      UInt8 status = _kernel.PortIn8(
        ATAPort(ioBase, ATARegister::Status)
      );

      _kernel.WriteLog(
        Core::LogLevel::Error,
        "ATA: setup28 poll DRQ timeout/error, status 0x%x LBA %u",
        status, lba
      );

      return false;
    }

    return true;
  }
}
