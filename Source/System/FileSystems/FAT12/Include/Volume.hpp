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

      /**
       * Returns the number of root directory entries.
       * @return
       *   Root directory entry count.
       */
      UInt32 GetRootEntryCount() const;

      /**
       * Reads a root directory entry by index.
       * @param index
       *   Entry index within the root directory.
       * @param entry
       *   Output entry to populate.
       * @param end
       *   True if the end of directory was reached.
       * @return
       *   True if a valid entry was returned.
       */
      bool ReadRootEntry(
        UInt32 index,
        ABI::FileSystem::DirectoryEntry& entry,
        bool& end
      );

      /**
       * Reads a directory entry by index from a directory cluster chain.
       * @param startCluster
       *   First cluster of the directory.
       * @param index
       *   Entry index within the directory.
       * @param entry
       *   Output entry to populate.
       * @param end
       *   True if the end of directory was reached.
       * @return
       *   True if a valid entry was returned.
       */
      bool ReadDirectoryEntry(
        UInt32 startCluster,
        UInt32 index,
        ABI::FileSystem::DirectoryEntry& entry,
        bool& end
      );

      /**
       * Finds a directory entry by name.
       * @param startCluster
       *   First cluster of the directory.
       * @param isRoot
       *   True if the directory is the root directory.
       * @param name
       *   Entry name to locate.
       * @param outCluster
       *   Receives the entry start cluster.
       * @param outAttributes
       *   Receives entry attributes.
       * @return
       *   True if the entry was found.
       */
      bool FindEntry(
        UInt32 startCluster,
        bool isRoot,
        CString name,
        UInt32& outCluster,
        UInt8& outAttributes
      );

      /**
       * Reads a FAT12 table entry.
       * @param cluster
       *   Cluster index to read.
       * @param nextCluster
       *   Receives the next cluster.
       * @return
       *   True if the FAT entry was read.
       */
      bool ReadFATEntry(UInt32 cluster, UInt32& nextCluster);

      /**
       * Returns true if the FAT entry marks end of chain.
       * @param value
       *   FAT entry value.
       * @return
       *   True if this value is end of chain.
       */
      static bool IsEndOfChain(UInt32 value);

    private:
      /**
       * Raw FAT directory entry.
       */
      struct DirectoryRecord {
        /**
         * 8.3 name field.
         */
        UInt8 name[11];

        /**
         * Attribute flags.
         */
        UInt8 attributes;

        /**
         * First cluster index.
         */
        UInt16 startCluster;

        /**
         * Size in bytes.
         */
        UInt32 sizeBytes;
      };

      /**
       * Boot sector LBA.
       */
      static constexpr UInt32 _bootSectorLBA = 0;

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

      /**
       * Case-insensitive name comparison.
       * @param left
       *   First name string.
       * @param right
       *   Second name string.
       * @return
       *   True if the names match.
       */
      static bool MatchName(CString left, CString right);

      /**
       * Formats a FAT 8.3 name into a display string.
       * @param base
       *   Pointer to the 11-byte name field.
       * @param outName
       *   Output name buffer.
       * @param outBytes
       *   Output buffer size in bytes.
       */
      static void FormatName(
        const UInt8* base,
        char* outName,
        UInt32 outBytes
      );

      /**
       * Reads a directory record from the root directory.
       * @param index
       *   Entry index within the root directory.
       * @param record
       *   Output record to populate.
       * @param end
       *   True if the end marker was reached.
       * @return
       *   True if a record was returned.
       */
      bool ReadRootRecord(UInt32 index, DirectoryRecord& record, bool& end);

      /**
       * Reads a directory record from a directory cluster chain.
       * @param startCluster
       *   First cluster of the directory.
       * @param index
       *   Entry index within the directory.
       * @param record
       *   Output record to populate.
       * @param end
       *   True if the end marker was reached.
       * @return
       *   True if a record was returned.
       */
      bool ReadDirectoryRecord(
        UInt32 startCluster,
        UInt32 index,
        DirectoryRecord& record,
        bool& end
      );

      /**
       * Converts a directory record to a directory entry.
       * @param record
       *   Record to convert.
       * @param entry
       *   Output entry to populate.
       * @return
       *   True if the entry is usable.
       */
      static bool RecordToEntry(
        const DirectoryRecord& record,
        ABI::FileSystem::DirectoryEntry& entry
      );
  };
}
