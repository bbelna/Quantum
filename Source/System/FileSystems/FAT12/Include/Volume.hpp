/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Include/Volume.hpp
 * FAT12 volume description.
 */

#pragma once

#include <ABI/Devices/BlockDevice.hpp>
#include <ABI/FileSystem.hpp>
#include <ABI/Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::FileSystems::FAT12 {
  /**
   * FAT12 volume state and geometry.
   */
  class Volume {
    public:
      /**
       * Loads the FAT12 volume metadata from disk.
       * @return
       *   True if the volume metadata loaded successfully.
       */
      bool Load();

      /**
       * Returns true if the volume metadata is valid.
       * @return
       *   True if the volume is valid.
       */
      bool IsValid() const;

      /**
       * Returns the volume info descriptor.
       * @return
       *   Reference to the volume info.
       */
      const ABI::FileSystem::VolumeInfo& GetInfo() const;

      /**
       * Returns the fixed volume handle.
       * @return
       *   The volume handle.
       */
      ABI::FileSystem::VolumeHandle GetHandle() const;

      /**
       * Checks whether the label matches this volume.
       * @param label
       *   Volume label to check.
       * @return
       *   True if the label matches.
       */
      bool MatchesLabel(CString label) const;

      /**
       * Fills a volume entry for listing.
       * @param entry
       *   Reference to the volume entry to fill.
       */
      void FillEntry(ABI::FileSystem::VolumeEntry& entry) const;

    private:
      /**
       * Boot sector LBA.
       */
      static constexpr UInt32 _bootSectorLba = 0;

      /**
       * Fixed handle for this volume.
       */
      static constexpr ABI::FileSystem::VolumeHandle _handle = 1;

      /**
       * Whether this volume has valid metadata.
       */
      bool _valid = false;

      /**
       * Backing block device info.
       */
      ABI::Devices::BlockDevice::Info _device {};

      /**
       * Cached volume info.
       */
      ABI::FileSystem::VolumeInfo _info {};

      /**
       * FAT region start LBA.
       */
      UInt32 _fatStartLBA = 0;

      /**
       * FAT size in sectors.
       */
      UInt32 _fatSectors = 0;

      /**
       * Root directory start LBA.
       */
      UInt32 _rootDirectoryStartLBA = 0;

      /**
       * Root directory size in sectors.
       */
      UInt32 _rootDirectorySectors = 0;

      /**
       * Data region start LBA.
       */
      UInt32 _dataStartLBA = 0;

      /**
       * Sectors per cluster.
       */
      UInt32 _sectorsPerCluster = 0;

      /**
       * Root directory entry count.
       */
      UInt32 _rootEntryCount = 0;

      /**
       * Reads the current boot sector into the provided buffer.
       * @param buffer
       *   Pointer to the buffer to fill.
       * @param bufferBytes
       *   Size of the buffer in bytes.
       * @return
       *   True if the boot sector was read successfully.
       */
      bool ReadBootSector(UInt8* buffer, UInt32 bufferBytes);

      /**
       * Locates the floppy block device.
       * @param outInfo
       *   Reference to receive the block device info.
       * @return
       *   True if a floppy block device was found.
       */
      bool GetFloppyInfo(ABI::Devices::BlockDevice::Info& outInfo);

      /**
       * Reads a little-endian 16-bit value.
       * @param base
       *   Pointer to the base buffer.
       * @param offset
       *   Offset within the buffer.
       * @return
       *   The 16-bit value.
       */
      static UInt16 ReadUInt16(const UInt8* base, UInt32 offset);

      /**
       * Reads a little-endian 32-bit value.
       * @param base
       *   Pointer to the base buffer.
       * @param offset
       *   Offset within the buffer.
       * @return
       *   The 32-bit value.
       */
      static UInt32 ReadUInt32(const UInt8* base, UInt32 offset);

      /**
       * Case-insensitive label comparison for single-letter volumes.
       * @param label
       *   Volume label to check.
       * @param expected
       *   Expected volume label.
       * @return
       *   True if the labels match.
       */
      static bool MatchLabel(CString label, CString expected);
  };
}
