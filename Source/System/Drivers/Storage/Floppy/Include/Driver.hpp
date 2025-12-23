/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.hpp
 * Quantum's floppy driver.
 */

#pragma once

#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/IPC.hpp>
#include <ABI/Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::Drivers::Storage::Floppy {
  using BlockDevice = ABI::Devices::BlockDevice;
  using IPC = ABI::IPC;

  /**
   * Floppy driver.
   */
  class Driver {
    public:
      /**
       * Entry point for the floppy driver.
       */
      static void Main();

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
       * Read data command (MFM, multi-track off).
       */
      static constexpr UInt8 _commandReadData = 0xE6;

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
      static constexpr UInt8 _sectorsPerTrack = 18;

      /**
       * Default head count for 1.44MB floppies.
       */
      static constexpr UInt8 _headCount = 2;

      /**
       * True after the controller is initialized.
       */
      inline static bool _initialized = false;

      /**
       * Maximum number of floppy devices to track.
       */
      static constexpr UInt32 _maxDevices = 2;

      /**
       * Bound block device identifiers.
       */
      inline static UInt32 _deviceIds[_maxDevices] = {};

      /**
       * Device sector sizes in bytes.
       */
      inline static UInt32 _deviceSectorSizes[_maxDevices] = {};

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
      inline static BlockDevice::Message _blockRequest {};

      /**
       * Prepared block response message.
       */
      inline static BlockDevice::Message _blockResponse {};

      /**
       * Waits for the controller FIFO to enter the desired phase.
       * @param readPhase
       *   True to wait for a read phase; false for write.
       * @return
       *   True if the controller is ready; false on timeout.
       */
      static bool WaitForFIFOReady(bool readPhase);

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
       * Programs the DMA controller for a floppy read.
       */
      static bool ProgramDMARead(UInt32 physicalAddress, UInt32 lengthBytes);

      /**
       * Selects the target drive and toggles the motor.
       */
      static void SetDrive(UInt8 driveIndex, bool motorOn);

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
       * Converts LBA to CHS for 1.44MB geometry.
       */
      static void LBAToCHS(
        UInt32 lba,
        UInt8& cylinder,
        UInt8& head,
        UInt8& sector
      );

      /**
       * Reads one or more sectors into the DMA buffer.
       */
      static bool ReadSectors(
        UInt8 driveIndex,
        UInt32 lba,
        UInt32 count,
        UInt32 sectorSize
      );

      /**
       * Registers a floppy device mapping.
        * @param info
       *   Block device info for the floppy.
       * @return
       *   True if the device was recorded; false otherwise.
       */
      static bool RegisterDevice(const BlockDevice::Info& info);

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
        UInt32& sectorSize
      );
  };
}
