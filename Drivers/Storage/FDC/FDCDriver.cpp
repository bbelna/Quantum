/**
 * @file Drivers/Storage/FDC/FDCDriver.cpp
 * @brief Implements @ref @QDrvs::Storage::FDC::FDCDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "FDCDriver.hpp"

namespace Quantum::Drivers::Storage::FDC {
  FDCDriver::FDCDriver(KernelClient& kernel) : _kernel(kernel) {}

  FDCDriver::~FDCDriver() {
    _motorOff();

    if (_irqHandle != static_cast<Kernel::Resources::ResourceID>(-1)) {
      _kernel.ReleaseInterrupt(_irqHandle);
    }

    if (_dmaBuffer) FreeBlock(_dmaBuffer);
  }

  bool FDCDriver::Probe() {
    // detect hypervisor via the CPU platform driver so we can skip
    // motor spin-up delays on emulated hardware
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
              LogLevel::Info,
              "Hypervisor detected, skipping motor delays"
            );
          }

          break;
        }
      }
    }

    _kernel.PortOut8(PortCCR, 0x00); // 500 kbps

    // allocate DMA bounce buffer; ISA DMA channel 2 uses a 24-bit physical
    // address, so the buffer must reside below 16 MB
    UIntPtr virtualAddress = _kernel.AllocateDMA(SectorSize * 8, ISADMALimit);

    if (virtualAddress == 0) return false;

    _dmaPhysical = _kernel.VirtualToPhysical(virtualAddress);
    _dmaBuffer = reinterpret_cast<void*>(virtualAddress);

    // claim IRQ 6 before reset so _reset() can use WaitForInterrupt
    _irqHandle = _kernel.ClaimInterrupt(6);

    if (_irqHandle == static_cast<Kernel::Resources::ResourceID>(-1)) {
      FreeBlock(virtualAddress);

      _dmaBuffer = nullptr;

      return false;
    }

    // reset and configure the FDC
    if (!_reset()) return false;

    // recalibrate each present drive to establish track 0
    for (UInt8 driveIndex = 0; driveIndex < 2; driveIndex++) {
      if (DrivePresent(driveIndex)) {
        _motorOn(driveIndex);
        _recalibrate(driveIndex);
      }
    }

    // start the motor stay-on timer; the server's idle loop will call
    // MotorIdleTick to eventually turn the motor off
    _motorCountdown = MotorStayOnTicks;

    return true;
  }

  bool FDCDriver::DrivePresent(UInt8 driveIndex) {
    if (driveIndex > 1) return false;

    _kernel.PortOut8(0x70, 0x10);

    UInt8 types = _kernel.PortIn8(0x71);
    UInt8 type = (driveIndex == 0) ? (types >> 4) : (types & 0x0F);

    return type != 0;
  }

  StorageABI::StorageOperationErrorCode FDCDriver::Read(
    UInt8 driveIndex,
    UInt64 lba,
    UInt32 count,
    void* buffer
  ) {
    if (!_dmaBuffer) return StorageABI::StorageOperationErrorCode::DeviceNotReady;

    UInt8* destination = static_cast<UInt8*>(buffer);
    UInt32 remaining = count;
    UInt64 currentLBA = lba;

    _motorOn(driveIndex);

    while (remaining > 0) {
      UInt8 cylinder, head, sector;

      _lbaToCHS(currentLBA, cylinder, head, sector);

      UInt32 trackLeft = SectorsPerTrack - sector + 1;
      UInt32 batch = remaining;

      if (batch > trackLeft) batch = trackLeft;
      if (batch > 8) batch = 8;

      bool ok = false;

      for (UInt8 attempt = 0; attempt < 3; attempt++) {
        if (!_seek(driveIndex, cylinder)) {
          _recalibrate(driveIndex);

          continue;
        }

        _setupDMA(false, batch);

        if (!_sendCommand(FDCCommand::ReadData)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand((head << 2) | driveIndex)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(cylinder)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(head)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(sector)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(0x02)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(SectorsPerTrack)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(0x1B)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(0xFF)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        }

        _kernel.WaitForInterrupt(_irqHandle);

        // result phase: exactly 7 bytes (ST0, ST1, ST2, C, H, R, N)
        UInt8 st0 = _readResult();
        UInt8 st1 = _readResult();
        UInt8 st2 = _readResult();

        for (UInt8 i = 0; i < 4; i++) _readResult(); // C, H, R, N

        if ((st0 & 0xC0) != 0x00 || st1 != 0x00 || st2 != 0x00) {
          _reset();
          _motorOn(driveIndex);
          _recalibrate(driveIndex);

          continue;
        }

        ok = true;

        break;
      }

      // If we failed 3 attempts in a row, give up on the read and return an
      // error
      if (!ok) {
        _motorOff();

        return StorageABI::StorageOperationErrorCode::ReadError;
      }

      const UInt8* source = static_cast<const UInt8*>(_dmaBuffer);
      UInt32 bytes = batch * SectorSize;

      for (UInt32 byteIndex = 0; byteIndex < bytes; byteIndex++) {
        destination[byteIndex] = source[byteIndex];
      }

      destination += bytes;
      currentLBA += batch;
      remaining -= batch;
    }

    // reset the stay-on timer so the motor remains spinning for follow-up I/O
    _motorCountdown = MotorStayOnTicks;

    return StorageABI::StorageOperationErrorCode::None;
  }

  StorageABI::StorageOperationErrorCode FDCDriver::Write(
    UInt8 driveIndex,
    UInt64 lba,
    UInt32 count,
    const void* buffer
  ) {
    if (!_dmaBuffer) return StorageABI::StorageOperationErrorCode::DeviceNotReady;

    const UInt8* source = static_cast<const UInt8*>(buffer);
    UInt32 remaining = count;
    UInt64 currentLBA = lba;

    _motorOn(driveIndex);

    while (remaining > 0) {
      UInt8 cylinder, head, sector;

      _lbaToCHS(currentLBA, cylinder, head, sector);

      UInt32 trackLeft = SectorsPerTrack - sector + 1;
      UInt32 batch = remaining;

      if (batch > trackLeft) batch = trackLeft;
      if (batch > 8) batch = 8;

      UInt32 bytes = batch * SectorSize;
      bool ok = false;

      for (UInt8 attempt = 0; attempt < 3; attempt++) {
        UInt8* dmaDestination = static_cast<UInt8*>(_dmaBuffer);

        for (UInt32 byteIndex = 0; byteIndex < bytes; byteIndex++) {
          dmaDestination[byteIndex] = source[byteIndex];
        }

        if (!_seek(driveIndex, cylinder)) {
          _recalibrate(driveIndex);

          continue;
        }

        _setupDMA(true, batch);

        if (!_sendCommand(FDCCommand::WriteData)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand((head << 2) | driveIndex)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(cylinder)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(head)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(sector)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(0x02)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(SectorsPerTrack)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(0x1B)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        } else if (!_sendCommand(0xFF)) {
          _reset();
          _motorOn(driveIndex);

          continue;
        }

        _kernel.WaitForInterrupt(_irqHandle);

        // result phase: exactly 7 bytes (ST0, ST1, ST2, C, H, R, N)
        UInt8 st0 = _readResult();
        UInt8 st1 = _readResult();
        UInt8 st2 = _readResult();

        for (UInt8 i = 0; i < 4; i++) _readResult(); // C, H, R, N

        if ((st0 & 0xC0) != 0x00 || st1 != 0x00 || st2 != 0x00) {
          _reset();
          _motorOn(driveIndex);
          _recalibrate(driveIndex);

          continue;
        }

        ok = true;

        break;
      }

      if (!ok) {
        _motorOff();

        return StorageABI::StorageOperationErrorCode::WriteError;
      }

      source += bytes;
      currentLBA += batch;
      remaining -= batch;
    }

    // reset the stay-on timer so the motor remains spinning for follow-up I/O
    _motorCountdown = MotorStayOnTicks;

    return StorageABI::StorageOperationErrorCode::None;
  }

  void FDCDriver::MotorIdleTick(UInt32 elapsedTicks) {
    if (!_motorRunning) {
      // track how long the motor has been off for the coast optimisation
      if (_motorOffElapsed < MotorCoastTicks) {
        _motorOffElapsed += elapsedTicks;
      }

      return;
    }

    if (elapsedTicks >= _motorCountdown) {
      _motorOff();
      _motorCountdown = 0;
    } else {
      _motorCountdown -= elapsedTicks;
    }
  }

  bool FDCDriver::_sendCommand(UInt8 byte) {
    for (UInt32 iteration = 0; iteration < FIFOTimeout; iteration++) {
      UInt8 msr = _kernel.PortIn8(PortMSR);

      if ((msr & static_cast<UInt8>(MSRFlag::MainRequest)) && !(msr & static_cast<UInt8>(MSRFlag::DataDirection))) {
        _kernel.PortOut8(PortFIFO, byte);

        return true;
      }
    }

    return false;
  }

  bool FDCDriver::_sendCommand(FDCCommand command) {
    return _sendCommand(static_cast<UInt8>(command));
  }

  UInt8 FDCDriver::_readResult() {
    for (UInt32 iteration = 0; iteration < FIFOTimeout; iteration++) {
      UInt8 msr = _kernel.PortIn8(PortMSR);

      if ((msr & static_cast<UInt8>(MSRFlag::MainRequest)) && (msr & static_cast<UInt8>(MSRFlag::DataDirection))) {
        return _kernel.PortIn8(PortFIFO);
      }
    }

    return 0xFF;
  }

  bool FDCDriver::_reset() {
    _kernel.PortOut8(PortDOR, 0x00);

    // let the FDC settle after asserting reset; instant on emulated hardware
    if (!_hypervisor) {
      for (UInt32 delay = 0; delay < 1000; delay++) asm volatile("" ::: "memory");
    }

    _kernel.PortOut8(PortDOR, static_cast<UInt8>(DORFlag::EnableDMAIRQ | DORFlag::Reset));

    _motorRunning = false;
    _currentCylinder[0] = 0xFF;
    _currentCylinder[1] = 0xFF;

    if (_irqHandle != static_cast<Kernel::Resources::ResourceID>(-1)) {
      _kernel.WaitForInterrupt(_irqHandle);
    } else if (!_hypervisor) {
      for (UInt32 delay = 0; delay < 10000; delay++) asm volatile("" ::: "memory");
    }

    for (UInt8 index = 0; index < 4; index++) _senseInterrupt();

    return _sendCommand(FDCCommand::Specify)
        && _sendCommand(0xDF)
        && _sendCommand(0x02);
  }

  bool FDCDriver::_recalibrate(UInt8 driveIndex) {
    if (
      !_sendCommand(FDCCommand::Recalibrate) ||
      !_sendCommand(driveIndex)
    ) return false;

    _kernel.WaitForInterrupt(_irqHandle);

    _senseInterrupt();

    _currentCylinder[driveIndex] = 0;

    return true;
  }

  bool FDCDriver::_seek(UInt8 driveIndex, UInt8 cylinder) {
    if (_currentCylinder[driveIndex] == cylinder) return true;

    if (
      !_sendCommand(FDCCommand::Seek) ||
      !_sendCommand((0 << 2) | driveIndex) ||
      !_sendCommand(cylinder)
    ) return false;

    _kernel.WaitForInterrupt(_irqHandle);

    _senseInterrupt();

    _currentCylinder[driveIndex] = cylinder;

    return true;
  }

  void FDCDriver::_senseInterrupt() {
    _sendCommand(FDCCommand::SenseInterrupt);
    _readResult();
    _readResult();
  }

  void FDCDriver::_motorOn(UInt8 driveIndex) {
    DORFlag motor = (driveIndex == 0) ? DORFlag::MotorA : DORFlag::MotorB;

    _kernel.PortOut8(
      PortDOR,
      static_cast<UInt8>(motor | DORFlag::EnableDMAIRQ | DORFlag::Reset)
        | driveIndex
    );

    if (!_motorRunning || _motorDrive != driveIndex) {
      if (!_hypervisor) {
        if (_motorOffElapsed < MotorCoastTicks && _motorDrive == driveIndex) {
          // platters are still coasting; a short settle is enough
          _kernel.SleepThread(MotorSpinUpTicks / 4);
        } else {
          // full spin-up from standstill
          _kernel.SleepThread(MotorSpinUpTicks);
        }
      }

      _motorRunning = true;
      _motorDrive = driveIndex;
    }
  }

  void FDCDriver::_motorOff() {
    _kernel.PortOut8(PortDOR, static_cast<UInt8>(DORFlag::EnableDMAIRQ | DORFlag::Reset));

    _motorRunning = false;
    _motorOffElapsed = 0;
  }

  void FDCDriver::_setupDMA(bool write, UInt32 sectors) {
    _kernel.PortOut8(DMAMask, 0x06); // mask channel 2
    _kernel.PortOut8(DMAFlipFlop, 0x00);
    _kernel.PortOut8(DMAAddress, static_cast<UInt8>(_dmaPhysical & 0xFF));
    _kernel.PortOut8(DMAAddress, static_cast<UInt8>((_dmaPhysical >> 8) & 0xFF));
    _kernel.PortOut8(DMAPage, static_cast<UInt8>((_dmaPhysical >> 16) & 0xFF));
    _kernel.PortOut8(DMAFlipFlop, 0x00);

    UInt16 transferCount = static_cast<UInt16>(sectors * SectorSize - 1);

    _kernel.PortOut8(DMACount, static_cast<UInt8>(transferCount & 0xFF));
    _kernel.PortOut8(DMACount, static_cast<UInt8>((transferCount >> 8) & 0xFF));

    // single mode, no auto-init, direction, channel 2
    UInt8 mode = write
      ? static_cast<UInt8>(0x4A) // read from memory (write to disk)
      : static_cast<UInt8>(0x46); // write to memory (read from disk)

    _kernel.PortOut8(DMAMode, mode);
    _kernel.PortOut8(DMAMask, 0x02); // unmask channel 2
  }

  void FDCDriver::_lbaToCHS(
    UInt64 lba,
    UInt8& cylinder,
    UInt8& head,
    UInt8& sector
  ) {
    cylinder = static_cast<UInt8>(
      lba / (static_cast<UInt64>(Heads * SectorsPerTrack))
    );
    head = static_cast<UInt8>((lba / SectorsPerTrack) % Heads);
    sector = static_cast<UInt8>((lba % SectorsPerTrack) + 1);
  }
}
