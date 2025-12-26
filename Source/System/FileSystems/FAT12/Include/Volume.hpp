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
        UInt8& outAttributes,
        UInt32& outSize
      );

      /**
       * Reads file data into a buffer.
       * @param startCluster
       *   First cluster of the file.
       * @param offset
       *   Byte offset within the file.
       * @param buffer
       *   Output buffer to populate.
       * @param length
       *   Maximum bytes to read.
       * @param outRead
       *   Receives the number of bytes read.
       * @param fileSize
       *   File size in bytes.
       * @return
       *   True if the read completed.
       */
      bool ReadFile(
        UInt32 startCluster,
        UInt32 offset,
        UInt8* buffer,
        UInt32 length,
        UInt32& outRead,
        UInt32 fileSize
      );

      /**
       * Writes file data from a buffer.
       * @param startCluster
       *   First cluster of the file (updated if needed).
       * @param offset
       *   Byte offset within the file.
       * @param buffer
       *   Input buffer to write.
       * @param length
       *   Number of bytes to write.
       * @param outWritten
       *   Receives the number of bytes written.
       * @param fileSize
       *   Current file size in bytes.
       * @param outSize
       *   Receives the updated file size.
       * @return
       *   True if the write completed.
       */
      bool WriteFileData(
        UInt32& startCluster,
        UInt32 offset,
        const UInt8* buffer,
        UInt32 length,
        UInt32& outWritten,
        UInt32 fileSize,
        UInt32& outSize
      );

      /**
       * Retrieves entry info by name.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   Entry name to locate.
       * @param outInfo
       *   Receives file info.
       * @param outAttributes
       *   Receives entry attributes.
       * @return
       *   True if the entry was found.
       */
      bool GetEntryInfo(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        ABI::FileSystem::FileInfo& outInfo,
        UInt8& outAttributes
      );

      /**
       * Retrieves the directory entry location.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   Entry name to locate.
       * @param outLBA
       *   Receives the entry LBA.
       * @param outOffset
       *   Receives the entry offset.
       * @return
       *   True if the entry was found.
       */
      bool GetEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Updates the size and start cluster for an entry.
       * @param lba
       *   Entry LBA.
       * @param offset
       *   Entry offset.
       * @param startCluster
       *   New start cluster.
       * @param sizeBytes
       *   New size in bytes.
       * @return
       *   True if the entry was updated.
       */
      bool UpdateEntry(
        UInt32 lba,
        UInt32 offset,
        UInt16 startCluster,
        UInt32 sizeBytes
      );

      /**
       * Creates a directory entry under the given parent.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   Directory name.
       * @return
       *   True if the directory was created.
       */
      bool CreateDirectory(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name
      );

      /**
       * Creates a file entry under the given parent.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   File name.
       * @return
       *   True if the file entry was created.
       */
      bool CreateFile(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name
      );

      /**
       * Removes an entry under the given parent.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   Entry name.
       * @return
       *   True if the entry was removed.
       */
      bool RemoveEntry(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name
      );

      /**
       * Renames an entry under the given parent.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   Existing entry name.
       * @param newName
       *   New entry name.
       * @return
       *   True if the entry was renamed.
       */
      bool RenameEntry(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        CString newName
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
       * Writes a FAT12 entry to disk.
       * @param cluster
       *   Cluster to update.
       * @param value
       *   FAT entry value.
       * @return
       *   True if the write succeeded.
       */
      bool WriteFATEntry(UInt32 cluster, UInt32 value);

      /**
       * Finds a free cluster.
       * @param outCluster
       *   Receives the free cluster id.
       * @return
       *   True if a free cluster was found.
       */
      bool FindFreeCluster(UInt32& outCluster);

      /**
       * Counts free clusters in the FAT.
       * @param outCount
       *   Receives the free cluster count.
       * @return
       *   True if the count was computed.
       */
      bool CountFreeClusters(UInt32& outCount);

      /**
       * Loads the FAT into a local cache.
       * @return
       *   True if the cache was loaded.
       */
      bool LoadFATCache();

      /**
       * Reads a cached FAT entry.
       * @param cluster
       *   Cluster index to read.
       * @param nextCluster
       *   Receives the next cluster.
       * @return
       *   True if the entry was read.
       */
      bool ReadFATEntryCached(UInt32 cluster, UInt32& nextCluster) const;

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
       * FAT table count.
       */
      UInt32 _fatCount = 0;

      /**
       * Cached FAT data.
       */
      UInt8 _fatCache[8192] = {};

      /**
       * Cached FAT size in bytes.
       */
      UInt32 _fatCacheBytes = 0;

      /**
       * Whether the FAT cache is valid.
       */
      bool _fatCached = false;

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
       * Next free cluster hint.
       */
      UInt32 _nextFreeCluster = 2;

      /**
       * Total cluster count.
       */
      UInt32 _clusterCount = 0;

      /**
       * Free cluster count.
       */
      UInt32 _freeClusters = 0;

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
       * Writes a little-endian 16-bit value.
       * @param base
       *   Pointer to the base buffer.
       * @param offset
       *   Offset within the buffer.
       * @param value
       *   Value to write.
       */
      static void WriteUInt16(UInt8* base, UInt32 offset, UInt16 value);

      /**
       * Writes a little-endian 32-bit value.
       * @param base
       *   Pointer to the base buffer.
       * @param offset
       *   Offset within the buffer.
       * @param value
       *   Value to write.
       */
      static void WriteUInt32(UInt8* base, UInt32 offset, UInt32 value);

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
       * Builds an 8.3 name field.
       * @param name
       *   Input name string.
       * @param outName
       *   Output 11-byte name buffer.
       * @return
       *   True if the name was valid.
       */
      static bool BuildShortName(CString name, UInt8* outName);

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
       * Finds a free directory slot in the parent.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is root.
       * @param outLBA
       *   Receives the LBA to write.
       * @param outOffset
       *   Receives the byte offset within the sector.
       * @return
       *   True if a slot was found.
       */
      bool FindFreeDirectorySlot(
        UInt32 parentCluster,
        bool parentIsRoot,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Computes the LBA and offset for a directory entry index.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is root.
       * @param index
       *   Entry index within the directory.
       * @param outLBA
       *   Receives the LBA.
       * @param outOffset
       *   Receives the entry offset within the sector.
       * @param end
       *   True if the index is beyond the directory.
       * @return
       *   True if the location was computed.
       */
      bool GetDirectoryEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        UInt32 index,
        UInt32& outLBA,
        UInt32& outOffset,
        bool& end
      );

      /**
       * Writes a directory entry to disk.
       * @param lba
       *   Sector LBA to write.
       * @param offset
       *   Byte offset within the sector.
       * @param entryBytes
       *   Entry bytes to write.
       * @return
       *   True if the entry was written.
       */
      bool WriteDirectoryEntry(
        UInt32 lba,
        UInt32 offset,
        const UInt8* entryBytes
      );

      /**
       * Finds a directory entry location.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is root.
       * @param name
       *   Entry name to locate.
       * @param record
       *   Receives the entry record.
       * @param outLBA
       *   Receives the entry LBA.
       * @param outOffset
       *   Receives the entry offset within the sector.
       * @return
       *   True if the entry was found.
       */
      bool FindEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        DirectoryRecord& record,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Updates the size and start cluster for an entry.
       * @param lba
       *   Entry LBA.
       * @param offset
       *   Entry offset.
       * @param startCluster
       *   New start cluster.
       * @param sizeBytes
       *   New size in bytes.
       * @return
       *   True if the entry was updated.
       */
      bool UpdateDirectoryEntry(
        UInt32 lba,
        UInt32 offset,
        UInt16 startCluster,
        UInt32 sizeBytes
      );

      /**
       * Marks a directory entry as deleted.
       * @param lba
       *   Entry LBA.
       * @param offset
       *   Entry offset.
       * @return
       *   True if the entry was marked deleted.
       */
      bool MarkEntryDeleted(UInt32 lba, UInt32 offset);

      /**
       * Renames a directory entry.
       * @param lba
       *   Entry LBA.
       * @param offset
       *   Entry offset.
       * @param shortName
       *   11-byte short name.
       * @return
       *   True if the entry was renamed.
       */
      bool RenameDirectoryEntry(
        UInt32 lba,
        UInt32 offset,
        const UInt8* shortName
      );

      /**
       * Frees a FAT cluster chain.
       * @param startCluster
       *   First cluster to free.
       * @return
       *   True if the chain was freed.
       */
      bool FreeClusterChain(UInt32 startCluster);

      /**
       * Returns true if a directory cluster is empty.
       * @param startCluster
       *   Directory start cluster.
       * @return
       *   True if only dot entries exist.
       */
      bool IsDirectoryEmpty(UInt32 startCluster);

      /**
       * Returns true if the record is "." or "..".
       * @param record
       *   Directory record to inspect.
       * @return
       *   True if the record is a dot entry.
       */
      static bool IsDotRecord(const DirectoryRecord& record);

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
