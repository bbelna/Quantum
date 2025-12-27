/**
 * @file System/Drivers/Storage/Floppy/Include/Driver.hpp
 * @brief Floppy driver.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright (c) 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <ABI/Devices/BlockDevices.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::Drivers::Storage::Floppy {
  using ABI::Devices::BlockDevices;
  using ABI::IPC;

  /**
   * Floppy driver.
   */
  class Driver {
    public:
      /**
       * Entry point for the floppy driver.
       */
      static void Main();

      /**
       * Locates the first floppy device and returns its info.
       * @param deviceId
       *   Receives the block device id.
       * @param info
       *   Receives the block device info.
       * @param driveIndex
       *   Receives the drive index.
       * @param sectorSize
       *   Receives the sector size.
       * @param sectorCount
       *   Receives the sector count.
       * @param sectorsPerTrack
       *   Receives the sectors per track.
       * @param headCount
       *   Receives the head count.
       * @return
       *   True if a device was found; false otherwise.
       */
      static bool GetDeviceInfo(
        UInt32& deviceId,
        BlockDevices::Info& info,
        UInt8& driveIndex,
        UInt32& sectorSize,
        UInt32& sectorCount,
        UInt8& sectorsPerTrack,
        UInt8& headCount
      );

      /**
       * Reads sectors directly via the controller.
       * @param driveIndex
       *   Target drive index.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to read.
       * @param sectorSize
       *   Sector size in bytes.
       * @param sectorsPerTrack
       *   Sectors per track.
       * @param headCount
       *   Head count.
       * @param outBuffer
       *   Destination buffer.
       * @param bufferBytes
       *   Size of the destination buffer in bytes.
       * @return
       *   True on success; false otherwise.
       */
      static bool ReadToBuffer(
        UInt8 driveIndex,
        UInt32 lba,
        UInt32 count,
        UInt32 sectorSize,
        UInt8 sectorsPerTrack,
        UInt8 headCount,
        void* outBuffer,
        UInt32 bufferBytes
      );

      /**
       * Writes sectors directly via the controller.
       * @param driveIndex
       *   Target drive index.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to write.
       * @param sectorSize
       *   Sector size in bytes.
       * @param sectorsPerTrack
       *   Sectors per track.
       * @param headCount
       *   Head count.
       * @param buffer
       *   Source buffer.
       * @param bufferBytes
       *   Size of the source buffer in bytes.
       * @return
       *   True on success; false otherwise.
       */
      static bool WriteFromBuffer(
        UInt8 driveIndex,
        UInt32 lba,
        UInt32 count,
        UInt32 sectorSize,
        UInt8 sectorsPerTrack,
        UInt8 headCount,
        const void* buffer,
        UInt32 bufferBytes
      );

    private:
      /**
       * Floppy digital output register port.
       */
      static constexpr UInt16 _digitalOutputRegisterPort = 0x3F2;

      /**
       * Floppy main status register port.
       */
      static constexpr UInt16 _mainStatusRegisterPort = 0x3F4;

      /**
       * Floppy data FIFO port.
       */
      static constexpr UInt16 _dataFIFOPort = 0x3F5;

      /**
       * DMA mask register port.
       */
      static constexpr UInt16 _dmaMaskPort = 0x0A;

      /**
       * DMA mode register port.
       */
      static constexpr UInt16 _dmaModePort = 0x0B;

      /**
       * DMA flip-flop reset port.
       */
      static constexpr UInt16 _dmaClearPort = 0x0C;

      /**
       * DMA channel 2 address port.
       */
      static constexpr UInt16 _dmaChannel2AddressPort = 0x04;

      /**
       * DMA channel 2 count port.
       */
      static constexpr UInt16 _dmaChannel2CountPort = 0x05;

      /**
       * DMA channel 2 page port.
       */
      static constexpr UInt16 _dmaChannel2PagePort = 0x81;

      /**
       * I/O access probe port (POST delay register).
       */
      static constexpr UInt16 _ioAccessProbePort = 0x80;

      /**
       * CMOS address register port.
       */
      static constexpr UInt16 _cmosAddressPort = 0x70;

      /**
       * CMOS data register port.
       */
      static constexpr UInt16 _cmosDataPort = 0x71;

      /**
       * CMOS floppy drive type register.
       */
      static constexpr UInt8 _cmosFloppyTypeRegister = 0x10;

      /**
       * Main status request/ready bit mask.
       */
      static constexpr UInt8 _mainStatusRequestMask = 0x80;

      /**
       * Main status data direction bit mask.
       */
      static constexpr UInt8 _mainStatusDirectionMask = 0x40;

      /**
       * DOR reset and DMA/IRQ enable bits.
       */
      static constexpr UInt8 _dorEnableMask = 0x0C;

      /**
       * Floppy motor enable mask for drive A.
       */
      static constexpr UInt8 _dorMotorA = 0x10;

      /**
       * Floppy motor enable mask for drive B.
       */
      static constexpr UInt8 _dorMotorB = 0x20;

      /**
       * DMA mode for channel 2 read (device -> memory).
       */
      static constexpr UInt8 _dmaModeRead = 0x46;

      /**
       * DMA mode for channel 2 write (memory -> device).
       */
      static constexpr UInt8 _dmaModeWrite = 0x4A;

      /**
       * Read data command (MFM, multi-track off).
       */
      static constexpr UInt8 _commandReadData = 0xE6;

      /**
       * Write data command (MFM, multi-track off).
       */
      static constexpr UInt8 _commandWriteData = 0xC5;

      /**
       * Read data command with multi-track enabled.
       */
      static constexpr UInt8 _commandReadDataMultiTrack = 0xE6 | 0x80;

      /**
       * Write data command with multi-track enabled.
       */
      static constexpr UInt8 _commandWriteDataMultiTrack = 0xC5 | 0x80;

      /**
       * IRQ line for the floppy controller.
       */
      static constexpr UInt32 _irqLine = 6;

      /**
       * Recalibrate command.
       */
      static constexpr UInt8 _commandRecalibrate = 0x07;

      /**
       * Seek command.
       */
      static constexpr UInt8 _commandSeek = 0x0F;

      /**
       * Default sectors per track for 1.44MB floppies.
       */
      static constexpr UInt8 _defaultSectorsPerTrack = 18;

      /**
       * Default head count for 1.44MB floppies.
       */
      static constexpr UInt8 _defaultHeadCount = 2;

      /**
       * True after the controller is initialized.
       */
      inline static bool _initialized = false;

      /**
       * Maximum number of floppy devices to track.
       */
      static constexpr UInt32 _maxDevices = 2;

      /**
       * Maximum read/write retry attempts.
       */
      static constexpr UInt32 _maxRetries = 3;

      /**
       * Bound block device identifiers.
       */
      inline static UInt32 _deviceIds[_maxDevices] = {};

      /**
       * Device sector sizes in bytes.
       */
      inline static UInt32 _deviceSectorSizes[_maxDevices] = {};

      /**
       * Device sector counts.
       */
      inline static UInt32 _deviceSectorCounts[_maxDevices] = {};

      /**
       * Device sectors per track.
       */
      inline static UInt8 _deviceSectorsPerTrack[_maxDevices] = {};

      /**
       * Device head counts.
       */
      inline static UInt8 _deviceHeadCounts[_maxDevices] = {};

      /**
       * Device indices (A=0, B=1).
       */
      inline static UInt8 _deviceIndices[_maxDevices] = {};

      /**
       * Number of bound devices.
       */
      inline static UInt32 _deviceCount = 0;

      /**
       * Default DMA buffer size in bytes.
       */
      static constexpr UInt32 _dmaBufferDefaultBytes = 4096;

      /**
       * DMA buffer physical address.
       */
      inline static UInt32 _dmaBufferPhysical = 0;

      /**
       * DMA buffer virtual address.
       */
      inline static UInt8* _dmaBufferVirtual = nullptr;

      /**
       * DMA buffer size in bytes.
       */
      inline static UInt32 _dmaBufferBytes = 0;

      /**
       * Cached cylinder per drive index.
       */
      inline static UInt8 _currentCylinder[_maxDevices] = {};

      /**
       * Pending floppy interrupt count.
       */
      inline static volatile UInt32 _irqPendingCount = 0;

      /**
       * Motor idle threshold (yield ticks before shutoff).
       */
      static constexpr UInt32 _motorIdleThreshold = 4000;

      /**
       * Motor state per drive index.
       */
      inline static bool _motorOn[_maxDevices] = {};

      /**
       * Motor idle counters per drive index.
       */
      inline static UInt32 _motorIdleCount[_maxDevices] = {};

      /**
       * IPC port id for this driver.
       */
      inline static UInt32 _portId = 0;

      /**
       * Maximum number of queued non-IRQ messages while waiting.
       */
      static constexpr UInt32 _maxPendingMessages = 4;

      /**
       * Pending IPC messages received during IRQ waits.
       */
      inline static IPC::Message _pendingMessages[_maxPendingMessages] = {};

      /**
       * Number of pending IPC messages.
       */
      inline static UInt32 _pendingCount = 0;

      /**
       * IPC receive buffer.
       */
      inline static IPC::Message _receiveMessage {};

      /**
       * IPC send buffer.
       */
      inline static IPC::Message _sendMessage {};

      /**
       * Parsed block request message.
       */
      inline static BlockDevices::Message _blockRequest {};

      /**
       * Prepared block response message.
       */
      inline static BlockDevices::Message _blockResponse {};

      /**
       * Waits for the controller FIFO to enter the desired phase.
       * @param readPhase
       *   True to wait for a read phase; false for write.
       * @return
       *   True if the controller is ready; false on timeout.
       */
      static bool WaitForFIFOReady(bool readPhase);

      /**
       * Reads a CMOS register.
       * @param reg
       *   CMOS register index.
       * @return
       *   Register value.
       */
      static UInt8 ReadCMOS(UInt8 reg);

      /**
       * Waits for the kernel to grant port I/O access.
       * @return
       *   True once I/O access is available; false on timeout.
       */
      static bool WaitForIOAccess();

      /**
       * Writes a byte into the controller FIFO.
       * @param value
       *   Byte to write.
       * @return
       *   True on success; false on timeout.
       */
      static bool WriteFIFOByte(UInt8 value);

      /**
       * Reads a byte from the controller FIFO.
       * @param value
       *   Receives the byte.
       * @return
       *   True on success; false on timeout.
       */
      static bool ReadFIFOByte(UInt8& value);

      /**
       * Issues the sense interrupt status command.
       * @param st0
       *   Receives the ST0 status byte.
       * @param cyl
       *   Receives the current cylinder number.
       * @return
       *   True on success; false on failure.
       */
      static bool SenseInterruptStatus(UInt8& st0, UInt8& cyl);

      /**
       * Resets the floppy controller.
       * @return
       *   True on success; false on failure.
       */
      static bool ResetController();

      /**
       * Sends the specify command to the controller.
       * @return
       *   True on success; false on failure.
       */
      static bool SendSpecifyCommand();

      /**
       * Copies raw bytes into a destination buffer.
       * @param dest
       *   Destination buffer.
       * @param src
       *   Source buffer.
       * @param length
       *   Number of bytes to copy.
       */
      static void CopyBytes(void* dest, const void* src, UInt32 length);

      /**
       * Copies bytes for IPC message parsing.
       * @param dest
       *   Destination buffer.
       * @param src
       *   Source buffer.
       * @param length
       *   Number of bytes to copy.
       */
      static void CopyMessageBytes(void* dest, const void* src, UInt32 length);

      /**
       * Checks whether an IPC message is a floppy IRQ notification.
       * @param msg
       *   IPC message to inspect.
       * @return
       *   True if the message is an IRQ notification; false otherwise.
       */
      static bool IsIRQMessage(const IPC::Message& msg);

      /**
       * Queues a non-IRQ message while waiting for an IRQ.
       * @param msg
       *   IPC message to queue.
       */
      static void QueuePendingMessage(const IPC::Message& msg);

      /**
       * Fills a buffer with a byte value.
       * @param dest
       *   Destination buffer.
       * @param value
       *   Byte value to fill.
       * @param length
       *   Number of bytes to fill.
       */
      static void FillBytes(void* dest, UInt8 value, UInt32 length);

      /**
       * Waits for a floppy IRQ to be delivered.
       */
      static bool WaitForIRQ();

      /**
       * Registers the IRQ route with the coordinator.
       */
      static void RegisterIRQRoute(UInt32 portId);

      /**
       * Sends a readiness signal to the coordinator.
       * @param deviceTypeId
       *   Device type identifier.
       */
      static void SendReadySignal(UInt8 deviceTypeId);

      /**
       * Programs the DMA controller for a floppy read.
       */
      static bool ProgramDMARead(UInt32 physicalAddress, UInt32 lengthBytes);

      /**
       * Programs the DMA controller for a floppy write.
       */
      static bool ProgramDMAWrite(UInt32 physicalAddress, UInt32 lengthBytes);

      /**
       * Selects the target drive and toggles the motor.
       */
      static void SetDrive(UInt8 driveIndex, bool motorOn);

      /**
       * Updates motor idle tracking and powers down as needed.
       */
      static void UpdateMotorIdle();

      /**
       * Waits for the motor to spin up.
       */
      static void WaitForMotorSpinUp();

      /**
       * Recalibrates a drive to cylinder 0.
       */
      static bool Calibrate(UInt8 driveIndex);

      /**
       * Seeks to a cylinder/head.
       */
      static bool Seek(UInt8 driveIndex, UInt8 cylinder, UInt8 head);

      /**
       * Converts LBA to CHS using the provided geometry.
       * @param lba
       *   Logical block address.
       * @param sectorsPerTrack
       *   Sectors per track for the drive.
       * @param headCount
       *   Number of heads for the drive.
       * @param cylinder
       *   Receives the cylinder.
       * @param head
       *   Receives the head.
       * @param sector
       *   Receives the sector (1-based).
       */
      static void LBAToCHS(
        UInt32 lba,
        UInt8 sectorsPerTrack,
        UInt8 headCount,
        UInt8& cylinder,
        UInt8& head,
        UInt8& sector
      );

      /**
       * Reads one or more sectors into the DMA buffer.
       * @param driveIndex
       *   Target drive index.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to read.
       * @param sectorSize
       *   Sector size in bytes.
       * @param sectorsPerTrack
       *   Sectors per track for the drive.
       * @param headCount
       *   Head count for the drive.
       */
      static bool ReadSectors(
        UInt8 driveIndex,
        UInt32 lba,
        UInt32 count,
        UInt32 sectorSize,
        UInt8 sectorsPerTrack,
        UInt8 headCount
      );

      /**
       * Writes one or more sectors from the DMA buffer.
       * @param driveIndex
       *   Target drive index.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to write.
       * @param sectorSize
       *   Sector size in bytes.
       * @param sectorsPerTrack
       *   Sectors per track for the drive.
       * @param headCount
       *   Head count for the drive.
       */
      static bool WriteSectors(
        UInt8 driveIndex,
        UInt32 lba,
        UInt32 count,
        UInt32 sectorSize,
        UInt8 sectorsPerTrack,
        UInt8 headCount
      );
      /**
       * Reads a little-endian 16-bit value from a buffer.
       * @param base
       *   Base pointer.
       * @param offset
       *   Offset in bytes.
       * @return
       *   16-bit value.
       */
      static UInt16 ReadUInt16(const UInt8* base, UInt32 offset);

      /**
       * Reads a little-endian 32-bit value from a buffer.
       * @param base
       *   Base pointer.
       * @param offset
       *   Offset in bytes.
       * @return
       *   32-bit value.
       */
      static UInt32 ReadUInt32(const UInt8* base, UInt32 offset);

      /**
       * Detects drive geometry from the boot sector.
       * @param driveIndex
       *   Drive index to probe.
       * @param outSectorSize
       *   Receives the detected sector size.
       * @param outSectorsPerTrack
       *   Receives the sectors per track.
       * @param outHeadCount
       *   Receives the head count.
       * @param outSectorCount
       *   Receives the total sector count.
       * @return
       *   True if geometry was detected; false otherwise.
       */
      static bool DetectGeometry(
        UInt8 driveIndex,
        UInt32& outSectorSize,
        UInt8& outSectorsPerTrack,
        UInt8& outHeadCount,
        UInt32& outSectorCount
      );

      /**
       * Registers a floppy device mapping.
       * @param info
       *   Block device info for the floppy.
       * @param sectorsPerTrack
       *   Detected sectors per track.
       * @param headCount
       *   Detected head count.
       * @return
       *   True if the device was recorded; false otherwise.
       */
      static bool RegisterDevice(
        const BlockDevices::Info& info,
        UInt8 sectorsPerTrack,
        UInt8 headCount
      );

      /**
       * Resolves a device id to its drive index and sector size.
       * @param deviceId
       *   Block device identifier.
       * @param driveIndex
       *   Receives the drive index (A=0, B=1).
       * @param sectorSize
       *   Receives the sector size in bytes.
       * @return
       *   True if the device was found; false otherwise.
       */
      static bool FindDevice(
        UInt32 deviceId,
        UInt8& driveIndex,
        UInt32& sectorSize,
        UInt32& sectorCount,
        UInt8& sectorsPerTrack,
        UInt8& headCount
      );

      /**
       * Writes a hexadecimal byte to the console.
       * @param value
       *   Byte value to write.
       */
      static void WriteHexByte(UInt8 value);

      /**
       * Writes a decimal unsigned integer to the console.
       * @param value
       *   Unsigned integer value to write.
       */
      static void WriteDecUInt(UInt32 value);

      /**
       * Logs result bytes from the controller for debugging.
       * @param result
       *   Result byte array.
       */
      static void LogResultBytes(const UInt8* result);

      /**
       * Logs a read failure message.
       * @param message
       *   Failure message.
       */
      static void LogReadFailure(CString message);

      /**
       * Logs the status of a calibrate attempt.
       * @param attempt
       *   Attempt number.
       * @param st0
       *   ST0 status byte.
       * @param cyl
       *   Current cylinder.
       */
      static void LogCalibrateStatus(UInt32 attempt, UInt8 st0, UInt8 cyl);
  };
}
