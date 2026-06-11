/**
 * @file Drivers/Storage/ATA/ATADriver.hpp
 * @brief Declares @ref @QDrvs::Storage::ATA::ATADriver.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <ATADriverTypes.hpp>

namespace Quantum::Drivers::Storage::ATA {
  /**
   * @brief ATA register offsets from the channel I/O base port.
   */
  enum class ATARegister : UInt16 {
    /**
     * @brief Data register (16-bit read/write).
     */
    Data = 0,

    /**
     * @brief Error register (read) / Features register (write).
     */
    Error = 1,

    /**
     * @brief Sector count.
     */
    SectorCount = 2,

    /**
     * @brief LBA bits 0-7.
     */
    LBALow = 3,

    /**
     * @brief LBA bits 8-15.
     */
    LBAMid = 4,

    /**
     * @brief LBA bits 16-23.
     */
    LBAHigh = 5,

    /**
     * @brief Drive/head register.
     */
    DriveHead = 6,

    /**
     * @brief Status register (read) / Command register (write).
     */
    Status = 7
  };

  /**
   * @brief Returns the I/O port address for a register on a given channel.
   * @param base Channel I/O base port.
   * @param reg Register offset.
   * @return The absolute I/O port address.
   */
  inline constexpr UInt16 ATAPort(UInt16 base, ATARegister reg) {
    return static_cast<UInt16>(base + static_cast<UInt16>(reg));
  }

  /**
   * @brief ATA status register bit flags.
   */
  enum class ATAStatus : UInt8 {
    /**
     * @brief Busy, controller is processing a command.
     */
    Busy = 0x80,

    /**
     * @brief Drive ready.
     */
    Ready = 0x40,

    /**
     * @brief Data request, data is ready to be transferred.
     */
    DataRequest = 0x08,

    /**
     * @brief Error flag.
     */
    Error = 0x01
  };

  /**
   * @brief ATA command bytes written to the command register.
   */
  enum class ATACommand : UInt8 {
    /**
     * @brief Read sectors (28-bit LBA, PIO).
     */
    ReadSectors = 0x20,

    /**
     * @brief Write sectors (28-bit LBA, PIO).
     */
    WriteSectors = 0x30,

    /**
     * @brief Read sectors via DMA (28-bit LBA).
     */
    ReadDMA = 0xC8,

    /**
     * @brief Write sectors via DMA (28-bit LBA).
     */
    WriteDMA = 0xCA,

    /**
     * @brief Flush write-back cache to disk.
     */
    FlushCache = 0xE7,

    /**
     * @brief Identify ATA device, returns 512 bytes of info.
     */
    Identify = 0xEC,

    /**
     * @brief Identify ATAPI device.
     */
    IdentifyPacket = 0xA1
  };

  /**
   * @brief Bus Master IDE register offsets from the channel's BM base.
   *
   * Each ATA channel has its own set of BM registers at an 8-byte offset:
   * primary at BM base + 0x00, secondary at BM base + 0x08.
   */
  enum class BMIDERegister : UInt8 {
    /**
     * @brief Command register. Bit 0 starts/stops the DMA engine;
     *        bit 3 selects read (1) or write (0) direction.
     */
    Command = 0x00,

    /**
     * @brief Status register. Bit 0 = active, bit 1 = error,
     *        bit 2 = interrupt pending.
     */
    Status = 0x02,

    /**
     * @brief Physical address of the PRD table (dword-aligned).
     */
    PRDTableAddress = 0x04
  };

  /**
   * @brief Physical Region Descriptor for the BMIDE scatter-gather table.
   *
   * Each PRD entry describes one contiguous physical memory region. The
   * table is terminated by setting bit 31 of @ref ByteCount in the last
   * entry (the EOT flag).
   */
  struct PRDEntry {
    /**
     * @brief Physical base address of the buffer (must not cross a 64 KB
     *        boundary).
     */
    UInt32 PhysicalAddress;

    /**
     * @brief Transfer byte count. Bit 31 is the End-Of-Table flag. A byte
     *        count of 0 means 64 KB (0x10000).
     */
    UInt32 ByteCount;
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::Drivers::Storage::ATA, ATAStatus
)

namespace Quantum::Drivers::Storage::ATA {
  /**
   * @brief ATA/IDE PIO mode driver for both the primary (`0x1F0`/IRQ 14) and
   *        secondary (`0x170`/IRQ 15) channels.
   *
   * Up to four drives are managed (two per channel):
   *   - Local index 0 = primary master,
   *   - Local index 1 = primary slave,
   *   - Local index 2 = secondary master, and
   *   - Local index 3 = secondary slave.
   *
   * All I/O uses 28-bit LBA PIO mode. `IDENTIFY DEVICE` is used during probing
   * to detect drive presence, type (ATA vs ATAPI), and capacity.
   */
  class ATADriver {
    public:
      /**
       * @brief Maximum number of sectors per DMA transfer. Limited by the
       *        bounce buffer size (64 KB = 128 sectors).
       */
      static constexpr UInt32 DMAMaxSectors = 128;

      /**
       * @brief Size of the DMA bounce buffer in bytes.
       */
      static constexpr UInt32 DMABufferSize = DMAMaxSectors * 512;

      /**
       * @brief Timeout limit for waiting on BSY to clear.
       */
      static constexpr UInt32 BsyTimeout = 0x100000;

      /**
       * @brief Reduced BSY timeout under a hypervisor (emulated controllers
       *        respond immediately; this only caps the cost of probing
       *        non-existent drives whose floating-bus signature fools the
       *        presence check).
       */
      static constexpr UInt32 BsyTimeoutHypervisor = 0x80;

      /**
       * @brief Timeout limit for waiting on DRQ to be asserted.
       */
      static constexpr UInt32 DrqTimeout = 0x100000;

      /**
       * @brief Reduced DRQ timeout under a hypervisor.
       */
      static constexpr UInt32 DrqTimeoutHypervisor = 0x80;

      /**
       * @brief Primary channel I/O base port.
       */
      static constexpr UInt16 PrimaryBase = 0x1F0;

      /**
       * @brief Primary channel control port.
       */
      static constexpr UInt16 PrimaryCtrl = 0x3F6;

      /**
       * @brief Secondary channel I/O base port.
       */
      static constexpr UInt16 SecondaryBase = 0x170;

      /**
       * @brief Secondary channel control port.
       */
      static constexpr UInt16 SecondaryCtrl = 0x376;

      /**
       * @brief Information about a single drive discovered during probing.
       */
      struct DriveInfo {
        /**
         * @brief `true` if a drive was found at this position.
         */
        bool Present = false;

        /**
         * @brief Device type: `HardDisk`, `OpticalDisk`, or `Unknown` for
         *        unsupported types.
         */
        StorageABI::StorageDeviceType Type =
          StorageABI::StorageDeviceType::Unknown;

        /**
         * @brief Total addressable sectors (28-bit LBA). Zero for ATAPI
         *        (optical) devices, which are read-only and use a different
         *        protocol.
         */
        UInt64 SectorCount = 0;

        /**
         * @brief Null-terminated model string extracted from `IDENTIFY` data
         *        (bytes 54-93), up to 40 characters.
         */
        char Model[41] = {};
      };

      /**
       * @brief Constructs the driver. No hardware is accessed until `Probe`
       *        is called.
       * @param kernel Reference to the @ref KernelClient. Must outlive this
       *               driver instance.
       */
      explicit ATADriver(KernelClient& kernel);

      /**
       * @brief Probes both ATA channels and populates the `DriveInfo` table.
       * @return `true` if at least one drive was found.
       */
      bool Probe();

      /**
       * @brief Returns drive information for the given local index.
       * @param localIndex `0`-`3` (`0` = primary master, `1` = primary slave,
       *                   `2` = secondary master, `3` = secondary slave).
       * @return Reference to the drive info entry.
       */
      const DriveInfo& GetDriveInfo(UInt8 localIndex) const;

      /**
       * @brief Returns whether the driver detected a hypervisor during
       *        probing.
       */
      bool IsHypervisor() const { return _hypervisor; }

      /**
       * @brief Reads one or more consecutive sectors into `buffer`.
       * @param localIndex  Drive index (`0`-`3`).
       * @param lba First sector to read.
       * @param count Number of sectors to read.
       * @param buffer Destination buffer (at least `count * 512` bytes).
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode Read(
        UInt8 localIndex,
        UInt64 lba,
        UInt32 count,
        void* buffer
      );

      /**
       * @brief Writes one or more consecutive sectors from `buffer`.
       * @param localIndex Drive index (`0`-`3`).
       * @param lba First sector to write.
       * @param count Number of sectors to write.
       * @param buffer Source buffer (at least `count * 512` bytes).
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode Write(
        UInt8 localIndex,
        UInt64 lba,
        UInt32 count,
        const void* buffer
      );

      /**
       * @brief Issues a FLUSH CACHE command to the specified drive.
       * @param localIndex Drive index (`0`-`3`).
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode Flush(UInt8 localIndex);

    private:
      /**
       * @brief Reference to the @ref KernelClient for system calls.
       */
      KernelClient& _kernel;

      /**
       * @brief `true` if running under a hypervisor. When set, 400 ns
       *        drive-select delays are skipped and polling timeouts are
       *        reduced since emulated ATA controllers respond instantly.
       */
      bool _hypervisor = false;

      /**
       * @brief Drive information for all four possible drives.
       */
      DriveInfo _drives[4];

      /**
       * @brief IRQ 14 handle for the primary ATA channel.
       */
      Kernel::Resources::ResourceID _primaryIRQHandle =
        static_cast<Kernel::Resources::ResourceID>(-1);

      /**
       * @brief IRQ 15 handle for the secondary ATA channel.
       */
      Kernel::Resources::ResourceID _secondaryIRQHandle =
        static_cast<Kernel::Resources::ResourceID>(-1);

      /**
       * @brief Returns the I/O base address for the channel that owns
       *        `localIndex`.
       */
      static constexpr UInt16 _base(UInt8 localIndex) {
        return (localIndex < 2) ? PrimaryBase : SecondaryBase;
      }

      /**
       * @brief Returns the control port address for the channel that owns
       *        `localIndex`.
       */
      static constexpr UInt16 _ctrl(UInt8 localIndex) {
        return (localIndex < 2) ? PrimaryCtrl : SecondaryCtrl;
      }

      /**
       * @brief Returns 0 for master drives (even local indices) and 1 for
       *        slave drives (odd local indices).
       */
      static constexpr UInt8 _driveOnChannel(UInt8 localIndex) {
        return localIndex & 1u;
      }

      /**
       * @brief Polls the status register until BSY clears.
       * @param base Channel I/O base port.
       * @return `true` if BSY cleared before timeout; `false` on timeout.
       */
      bool _waitBSY(UInt16 base);

      /**
       * @brief Polls until DRQ is set (data ready) or an error occurs.
       * @param base Channel I/O base port.
       * @return `true` if DRQ set without error; `false` on error or timeout.
       */
      bool _waitDRQ(UInt16 base);

      /**
       * @brief Selects a drive on its channel and waits for BSY to clear.
       * @param localIndex Drive index (`0`-`3`).
       */
      void _select(UInt8 localIndex);

      /**
       * @brief Issues the 28-bit LBA parameters and command byte for one
       *        sector, then waits for DRQ.
       * @param localIndex Drive index (`0`-`3`).
       * @param lba 28-bit sector address.
       * @param command `ATACommand::ReadSectors` or `ATACommand::WriteSectors`.
       * @return `true` on success.
       */
      bool _setup28(UInt8 localIndex, UInt32 lba, ATACommand command);

      /**
       * @brief Returns the IRQ handle for the channel that owns `localIndex`.
       * @param localIndex Drive index (`0`-`3`).
       * @return The IRQ resource handle, or -1 if no IRQ was claimed.
       */
      Kernel::Resources::ResourceID _irqHandle(UInt8 localIndex) const;

      /**
       * @brief Probes one channel (indices `base` and `base+1`).
       * @param base First local index for this channel (`0` for primary,
       *             `2` for secondary).
       */
      void _probeChannel(UInt8 base);

      // ----- Bus Master DMA -----

      /**
       * @brief `true` if PCI BMIDE was discovered and initialised.
       */
      bool _dmaAvailable = false;

      /**
       * @brief I/O base address for the primary channel's BM registers
       *        (BAR4 & 0xFFFFFFFC).
       */
      UInt16 _bmidePrimaryBase = 0;

      /**
       * @brief I/O base address for the secondary channel's BM registers
       *        (BAR4 + 0x08).
       */
      UInt16 _bmideSecondaryBase = 0;

      /**
       * @brief Virtual address of the PRD table (dword-aligned, physically
       *        contiguous).
       */
      PRDEntry* _prdTable = nullptr;

      /**
       * @brief Physical address of the PRD table.
       */
      UInt32 _prdTablePhysical = 0;

      /**
       * @brief Virtual address of the DMA bounce buffer (64 KB).
       */
      void* _dmaBuffer = nullptr;

      /**
       * @brief Physical address of the DMA bounce buffer.
       */
      UInt32 _dmaBufferPhysical = 0;

      /**
       * @brief Discovers the PCI IDE controller, reads BAR4, enables bus
       *        mastering, and allocates the PRD table and DMA buffer.
       *        Called from @ref Probe.
       */
      void _probeBMIDE();

      /**
       * @brief Returns the BMIDE register base for the channel that owns
       *        `localIndex`.
       */
      UInt16 _bmideBase(UInt8 localIndex) const {
        return (localIndex < 2) ? _bmidePrimaryBase : _bmideSecondaryBase;
      }

      /**
       * @brief Reads sectors using Bus Master DMA.
       * @param localIndex Drive index (`0`-`3`).
       * @param lba First sector to read.
       * @param count Number of sectors to read (max @ref DMAMaxSectors).
       * @param buffer Destination buffer.
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode _readDMA(
        UInt8 localIndex,
        UInt64 lba,
        UInt32 count,
        void* buffer
      );

      /**
       * @brief Writes sectors using Bus Master DMA.
       * @param localIndex Drive index (`0`-`3`).
       * @param lba First sector to write.
       * @param count Number of sectors to write (max @ref DMAMaxSectors).
       * @param buffer Source buffer.
       * @return `StorageABI::Error::None` on success.
       */
      StorageABI::StorageOperationErrorCode _writeDMA(
        UInt8 localIndex,
        UInt64 lba,
        UInt32 count,
        const void* buffer
      );

      /**
       * @brief Programs the PRD table for a single contiguous transfer
       *        from the DMA bounce buffer.
       * @param byteCount Number of bytes to transfer.
       */
      void _setupPRD(UInt32 byteCount);
  };
}
