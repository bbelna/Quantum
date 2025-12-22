/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Drivers/Storage/Floppy/Driver.hpp
 * User-mode floppy driver entry.
 */

#pragma once

#include <ABI/Devices/Block.hpp>
#include <ABI/IPC.hpp>
#include <Types.hpp>

namespace Quantum::System::Drivers::Storage::Floppy {
  /**
   * Floppy driver.
   */
  class Driver {
    public:
      /**
       * Entry point for the floppy driver service.
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
       * True after the controller is initialized.
       */
      static bool _initialized;

      /**
       * Maximum number of floppy devices to track.
       */
      static constexpr UInt32 _maxDevices = 2;

      /**
       * Bound block device identifiers.
       */
      static UInt32 _deviceIds[_maxDevices];

      /**
       * Device sector sizes in bytes.
       */
      static UInt32 _deviceSectorSizes[_maxDevices];

      /**
       * Device indices (A=0, B=1).
       */
      static UInt8 _deviceIndices[_maxDevices];

      /**
       * Number of bound devices.
       */
      static UInt32 _deviceCount;

      /**
       * Pending floppy interrupt count.
       */
      static volatile UInt32 _irqPendingCount;

      /**
       * IPC receive buffer.
       */
      static ABI::IPC::Message _receiveMessage;

      /**
       * IPC send buffer.
       */
      static ABI::IPC::Message _sendMessage;

      /**
       * Parsed block request message.
       */
      static ABI::Devices::Block::Message _blockRequest;

      /**
       * Prepared block response message.
       */
      static ABI::Devices::Block::Message _blockResponse;

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
       * Registers a floppy device mapping.
        * @param info
       *   Block device info for the floppy.
       * @return
       *   True if the device was recorded; false otherwise.
       */
      static bool RegisterDevice(const ABI::Devices::Block::Info& info);

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
