/**
 * @file Drivers/Storage/FDC/FDCDriver.hpp
 * @brief Declares @ref @QDrvs::Storage::FDC::FDCDriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <FDCDriverTypes.hpp>

namespace Quantum::Drivers::Storage::FDC {
  /**
   * @brief FDC command bytes sent via the data FIFO.
   */
  enum class FDCCommand : UInt8 {
    /**
     * @brief SPECIFY, set drive timing parameters.
     */
    Specify = 0x03,

    /**
     * @brief SENSE DRIVE STATUS.
     */
    SenseDrive = 0x04,

    /**
     * @brief RECALIBRATE, seek to track 0.
     */
    Recalibrate = 0x07,

    /**
     * @brief SENSE INTERRUPT STATUS.
     */
    SenseInterrupt = 0x08,

    /**
     * @brief SEEK.
     */
    Seek = 0x0F,

    /**
     * @brief READ DATA (MFM).
     */
    ReadData = 0xE6,

    /**
     * @brief WRITE DATA (MFM).
     */
    WriteData = 0xC5
  };

  /**
   * @brief Main Status Register bit flags.
   */
  enum class MSRFlag : UInt8 {
    /**
     * @brief Main Request, FIFO ready for command or data byte.
     */
    MainRequest = 0x80,

    /**
     * @brief Data I/O direction: 1 = FDC -> CPU, 0 = CPU -> FDC.
     */
    DataDirection = 0x40
  };

  /**
   * @brief Digital Output Register bit flags.
   */
  enum class DORFlag : UInt8 {
    /**
     * @brief Motor A enable.
     */
    MotorA = 0x10,

    /**
     * @brief Motor B enable.
     */
    MotorB = 0x20,

    /**
     * @brief Enable DMA and IRQ.
     */
    EnableDMAIRQ = 0x08,

    /**
     * @brief Release reset (normal operation).
     */
    Reset = 0x04
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Drivers::Storage::FDC, DORFlag
)

namespace Quantum::Drivers::Storage::FDC {
  /**
   * @brief Low-level driver for the primary FDC (Floppy Disk Controller) at
   *        I/O base address 0x3F0.
   *
   * The driver uses ISA DMA channel 2 for all data transfers. A 4 KB bounce
   * buffer is allocated at initialization time; its physical address is
   * resolved via @ref KernelClient::VirtualToPhysical and cached for the
   * lifetime of the driver. All sector reads and writes flow through this
   * buffer:
   *   - Read:  FDC DMA -> bounce buffer -> shared memory buffer.
   *   - Write: shared memory buffer -> bounce buffer -> FDC DMA.
   *
   * @note ISA DMA channel 2 uses a 16-bit address register plus an 8-bit page
   *       register, giving a 24-bit physical address. A 4 KB aligned page
   *       allocated from the kernel's normal pool is always below 16 MB and
   *       safe for ISA DMA use.
   */
  class FDCDriver {
    public:
      /**
       * @brief Timeout limit for FIFO ready polling.
       */
      static constexpr UInt32 ISADMALimit = 0x1000000;

      /**
       * @brief Timeout limit for FIFO ready polling.
       */
      static constexpr UInt32 FIFOTimeout = 0x10000;

      /**
       * @brief Motor spin-up delay in ticks (1 tick = 1ms at 1000 Hz PIT).
       */
      static constexpr UInt32 MotorSpinUpTicks = 500;

      /**
       * @brief Grace period after motor-off during which the platters are
       *        still coasting fast enough to skip the full spin-up delay.
       */
      static constexpr UInt32 MotorCoastTicks = 2000;

      /**
       * @brief Duration in ticks to keep the motor spinning after the last
       *        I/O operation before automatically turning it off. Thirty
       *        seconds keeps the drive hot through typical loading sequences
       *        where processing gaps between reads can be significant.
       */
      static constexpr UInt32 MotorStayOnTicks = 30000;

      /**
       * @brief Digital Output Register, motor on/off, drive select, reset.
       */
      static constexpr UInt16 PortDOR = 0x3F2;

      /**
       * @brief Main Status Register (read-only), FIFO ready flags.
       */
      static constexpr UInt16 PortMSR = 0x3F4;

      /**
       * @brief Data FIFO, command bytes in, result bytes out.
       */
      static constexpr UInt16 PortFIFO = 0x3F5;

      /**
       * @brief Configuration Control Register (write-only).
       */
      static constexpr UInt16 PortCCR = 0x3F7;

      /**
       * @brief Channel 2 base-address register (word, two bytes).
       */
      static constexpr UInt16 DMAAddress = 0x04;

      /**
       * @brief Channel 2 transfer-count register (word, two bytes).
       */
      static constexpr UInt16 DMACount = 0x05;

      /**
       * @brief Flip-flop reset.
       */
      static constexpr UInt16 DMAFlipFlop = 0x0C;

      /**
       * @brief Single-channel mask register.
       */
      static constexpr UInt16 DMAMask = 0x0A;

      /**
       * @brief Mode register.
       */
      static constexpr UInt16 DMAMode = 0x0B;

      /**
       * @brief Channel 2 page register (bits 16-23 of address).
       */
      static constexpr UInt16 DMAPage = 0x81;

      /**
       * @brief Number of cylinders.
       */
      static constexpr UInt8 Cylinders = 80;

      /**
       * @brief Number of heads (sides).
       */
      static constexpr UInt8 Heads = 2;

      /**
       * @brief Sectors per track.
       */
      static constexpr UInt8 SectorsPerTrack = 18;

      /**
       * @brief Bytes per sector.
       */
      static constexpr UInt32 SectorSize = 512;

      /**
       * @brief Total sectors on a 1.44 MB disk.
       */
      static constexpr UInt64 TotalSectors =
        static_cast<UInt64>(Cylinders) * Heads * SectorsPerTrack;

      /**
       * @brief Constructs the driver without accessing hardware.
       * @param kernel Reference to the @ref KernelClient. Must outlive this
       *               driver instance.
       */
      explicit FDCDriver(KernelClient& kernel);

      /**
       * @brief Destructs the driver, releasing the IRQ handle and DMA buffer.
       */
      ~FDCDriver();

      /**
       * @brief Detects the FDC, resets it, and allocates the DMA bounce
       *        buffer.
       * @return `true` if an FDC was detected and initialised successfully.
       */
      bool Probe();

      /**
       * @brief Queries the CMOS floppy-type field to determine whether a
       *        specific drive is physically installed.
       * @param driveIndex Drive index (0 = A:, 1 = B:).
       * @return `true` if the CMOS indicates a drive is present.
       */
      bool DrivePresent(UInt8 driveIndex);

      /**
       * @brief Returns whether the driver detected a hypervisor during
       *        probing.
       */
      bool IsHypervisor() const { return _hypervisor; }

      /**
       * @brief Called periodically from the server's idle path to manage the
       *        motor stay-on timer. If the motor is running and the countdown
       *        has expired, the motor is turned off.
       * @param elapsedTicks Number of ticks elapsed since the last call.
       */
      void MotorIdleTick(UInt32 elapsedTicks);

      /**
       * @brief Reads one or more consecutive sectors from a drive into
       *        `buffer`.
       * @param driveIndex Drive index (0 or 1).
       * @param lba First sector to read.
       * @param count Number of sectors to read.
       * @param buffer Destination (at least `count * SectorSize` bytes).
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode Read(
        UInt8 driveIndex,
        UInt64 lba,
        UInt32 count,
        void* buffer
      );

      /**
       * @brief Writes one or more consecutive sectors from `buffer` to a
       *        drive.
       * @param driveIndex Drive index (0 or 1).
       * @param lba First sector to write.
       * @param count Number of sectors to write.
       * @param buffer Source (at least `count * SectorSize` bytes).
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode Write(
        UInt8 driveIndex,
        UInt64 lba,
        UInt32 count,
        const void* buffer
      );

    private:
      /**
       * @brief Reference to the @ref KernelClient for system calls.
       */
      KernelClient& _kernel;

      /**
       * @brief IRQ 6 handle, claimed at `Probe` time.
       */
      Kernel::Resources::ResourceID _irqHandle =
        static_cast<Kernel::Resources::ResourceID>(-1);

      /**
       * @brief Virtual address of the 4 KB DMA bounce buffer.
       */
      void* _dmaBuffer = nullptr;

      /**
       * @brief Physical address of the DMA bounce buffer.
       */
      UInt32 _dmaPhysical = 0;

      /**
       * @brief Current cylinder for each drive to minimise seeks.
       */
      UInt8 _currentCylinder[2] = { 0xFF, 0xFF };

      /**
       * @brief `true` if running under a hypervisor. When set, motor
       *        spin-up delays are skipped since emulated FDC motors are
       *        instant.
       */
      bool _hypervisor = false;

      /**
       * @brief Whether a motor is currently spinning.
       */
      bool _motorRunning = false;

      /**
       * @brief Which drive's motor is currently spinning (0 or 1).
       */
      UInt8 _motorDrive = 0xFF;

      /**
       * @brief Remaining ticks before the motor is automatically turned off.
       *        Reset to @ref MotorStayOnTicks after each I/O operation.
       */
      UInt32 _motorCountdown = 0;

      /**
       * @brief Ticks elapsed since the motor was last turned off. Used to
       *        determine whether the platters are still coasting fast enough
       *        to skip the full spin-up delay.
       */
      UInt32 _motorOffElapsed = 0;

      /**
       * @brief Sends a single byte to the FDC FIFO.
       * @param byte The byte to send.
       * @return `true` if the byte was accepted before timeout.
       */
      bool _sendCommand(UInt8 byte);

      /**
       * @brief Sends an FDC command byte to the FIFO.
       * @param command The FDC command to send.
       * @return `true` if the command was accepted before timeout.
       */
      bool _sendCommand(FDCCommand command);

      /**
       * @brief Reads a single result byte from the FDC FIFO.
       * @return The result byte, or `0xFF` on timeout.
       */
      UInt8 _readResult();

      /**
       * @brief Resets the FDC and reconfigures drive timing.
       * @return `true` if the reset completed successfully.
       */
      bool _reset();

      /**
       * @brief Recalibrates a drive (seek to track 0).
       * @param driveIndex Drive index (0 or 1).
       * @return `true` on success.
       */
      bool _recalibrate(UInt8 driveIndex);

      /**
       * @brief Seeks to the specified cylinder on a drive.
       * @param driveIndex Drive index (0 or 1).
       * @param cylinder Target cylinder number.
       * @return `true` on success.
       */
      bool _seek(UInt8 driveIndex, UInt8 cylinder);

      /**
       * @brief Issues a SENSE INTERRUPT STATUS command and discards the
       *        result bytes.
       */
      void _senseInterrupt();

      /**
       * @brief Turns on the motor for the specified drive and waits for
       *        spin-up if necessary.
       * @param driveIndex Drive index (0 or 1).
       */
      void _motorOn(UInt8 driveIndex);

      /**
       * @brief Turns off the currently running motor.
       */
      void _motorOff();

      /**
       * @brief Configures ISA DMA channel 2 for a transfer.
       * @param write `true` for a write (memory -> disk), `false` for a read.
       * @param sectors Number of sectors to transfer.
       */
      void _setupDMA(bool write, UInt32 sectors);

      /**
       * @brief Converts an LBA sector address to CHS (cylinder, head,
       *        sector).
       * @param lba The logical block address.
       * @param cylinder Output cylinder number.
       * @param head Output head number.
       * @param sector Output sector number (1-based).
       */
      static void _lbaToCHS(
        UInt64 lba,
        UInt8& cylinder,
        UInt8& head,
        UInt8& sector
      );
  };
}
