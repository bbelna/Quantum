/**
 * @file FileSystems/FAT12/Volume.hpp
 * @brief Declares the FAT12 volume driver used by the FAT12 file system server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Enum.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Servers/FileSystem/ABI.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::FileSystems::FAT12 {
  /**
   * @brief FAT12 directory entry attribute flags.
   */
  enum class DirectoryAttribute : UInt8 {
    /**
     * @brief No attributes set.
     */
    None = 0x00,

    /**
     * @brief File is read-only.
     */
    ReadOnly = 0x01,

    /**
     * @brief File is hidden from normal directory listings.
     */
    Hidden = 0x02,

    /**
     * @brief File is a system file.
     */
    System = 0x04,

    /**
     * @brief Entry contains the volume label (name + extension fields).
     */
    VolumeLabel = 0x08,

    /**
     * @brief Entry is a subdirectory.
     */
    Directory = 0x10,

    /**
     * @brief File has been modified since the last backup.
     */
    Archive = 0x20,

    /**
     * @brief Long filename entry marker
     *        (`ReadOnly | Hidden | System | VolumeLabel`).
     */
    LongFileName = 0x0F
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(
  Quantum::FileSystems::FAT12, DirectoryAttribute
)

namespace Quantum::FileSystems::FAT12 {
  /**
   * @brief First byte of `Name` indicating the directory entry has been
   *        deleted and can be reused.
   */
  static constexpr UInt8 DirectoryEntryDeleted = 0xE5;

  /**
   * @brief First byte of `Name` indicating no more directory entries follow.
   */
  static constexpr UInt8 DirectoryEntryEnd = 0x00;

  /**
   * @brief Cluster values at or above this threshold mark the end of a
   *        cluster chain.
   */
  static constexpr UInt16 ClusterEOF = 0xFF8;

  /**
   * @brief Maximum characters in a FAT long filename (FAT spec limit).
   */
  static constexpr UInt32 LFNMaxLength = 255;

  /**
   * @brief Raw 32-byte FAT12 directory entry, matching the on-disk layout.
   */
  struct RawDirectoryEntry {
    /**
     * @brief 8-character filename, space-padded. The first byte doubles as
     *        a sentinel (`DirectoryEntryDeleted`, `DirectoryEntryEnd`).
     */
    char Name[8];

    /**
     * @brief 3-character file extension, space-padded.
     */
    char Extension[3];

    /**
     * @brief Attribute flags (`DirectoryAttribute` bitmask).
     */
    UInt8 Attributes;

    /**
     * @brief Reserved for Windows NT (case information). Not used by FAT12.
     */
    UInt8 NTReserved;

    /**
     * @brief Creation time in tenths of a second (0-199).
     */
    UInt8 CreationTimeTenths;

    /**
     * @brief Creation time (hours/minutes/seconds packed in 16 bits).
     */
    UInt16 CreationTime;

    /**
     * @brief Creation date (year/month/day packed in 16 bits).
     */
    UInt16 CreationDate;

    /**
     * @brief Last access date (year/month/day packed in 16 bits).
     */
    UInt16 LastAccessDate;

    /**
     * @brief High 16 bits of the first cluster number. Always 0 on FAT12.
     */
    UInt16 FirstClusterHigh;

    /**
     * @brief Last write time (hours/minutes/seconds packed in 16 bits).
     */
    UInt16 WriteTime;

    /**
     * @brief Last write date (year/month/day packed in 16 bits).
     */
    UInt16 WriteDate;

    /**
     * @brief Low 16 bits of the first data cluster. 0 for empty files.
     */
    UInt16 FirstCluster;

    /**
     * @brief File size in bytes. 0 for directories.
     */
    UInt32 FileSize;
  } __attribute__((packed));

  static_assert(
    sizeof(RawDirectoryEntry) == 32,
    "RawDirectoryEntry must be 32 bytes"
  );

  /**
   * @brief Raw 32-byte FAT Long Filename (LFN) directory entry. Shares the
   *        same 32-byte slot as a `RawDirectoryEntry` but has
   *        `Attributes == DirectoryAttribute::LongFileName`.
   */
  struct RawLFNDirectoryEntry {
    /**
     * @brief Sequence number (1-based). Bit 6 set on the first physical
     *        LFN entry of a new name; bits 0-4 hold the ordinal.
     */
    UInt8 Sequence;

    /**
     * @brief UTF-16LE characters 1-5 of this LFN fragment.
     */
    UInt16 Name1[5];

    /**
     * @brief Attribute byte. Always `0x0F`
     *        (`DirectoryAttribute::LongFileName`).
     */
    UInt8 Attributes;

    /**
     * @brief Entry type. Always 0 for VFAT LFN entries.
     */
    UInt8 Type;

    /**
     * @brief Checksum of the associated 8.3 short name.
     */
    UInt8 Checksum;

    /**
     * @brief UTF-16LE characters 6-11 of this LFN fragment.
     */
    UInt16 Name2[6];

    /**
     * @brief First cluster low word. Always 0 for LFN entries.
     */
    UInt16 FirstClusterLow;

    /**
     * @brief UTF-16LE characters 12-13 of this LFN fragment.
     */
    UInt16 Name3[2];
  } __attribute__((packed));

  static_assert(
    sizeof(RawLFNDirectoryEntry) == 32,
    "RawLFNDirectoryEntry must be 32 bytes"
  );

  /**
   * @brief A resolved FAT12 directory entry ready for use by the server.
   */
  struct ResolvedEntry {
    /**
     * @brief Null-terminated human-readable name (e.g. `"KERNEL.SYS"`).
     */
    char Name[Servers::FileSystem::ABI::FileSystemMaxFileNameLength];

    /**
     * @brief File size in bytes; 0 for directories.
     */
    UInt32 FileSize;

    /**
     * @brief First cluster of the file or directory data. `0` = FAT12 root
     *        directory (special case, fixed location, not a cluster chain).
     */
    UInt16 FirstCluster;

    /**
     * @brief `true` if this entry is a directory.
     */
    bool IsDirectory;

    /**
     * @brief `true` if this structure contains a valid entry.
     */
    bool Valid;
  };

  /**
   * @brief Mounts and queries a FAT12 file system on a single storage device.
   *
   * On `Mount`, the BPB is read from sector 0 and key layout parameters are
   * cached. The entire first FAT is loaded into heap memory and kept for the
   * lifetime of the volume, for a 1.44 MB floppy this is only `9 * 512 = 4608`
   * bytes.
   *
   * All sector reads go through `Storage::ABI::Read` with a short-lived shared
   * buffer (create -> attach -> read -> copy -> detach cycle per call).
   */
  class Volume {
    public:
      /**
       * @brief Destroys the volume, freeing the cached FAT buffer.
       */
      ~Volume();

      /**
       * @brief Attempts to mount a FAT12 file system on the given device.
       * @param deviceID Storage device ID (from `Storage::ABI::GetDevices`).
       * @return `true` if the BPB was valid and the FAT loaded successfully.
       */
      bool Mount(UInt32 deviceID);

      /**
       * @brief Returns whether the volume is currently mounted.
       */
      bool IsMounted() const { return _mounted; }

      /**
       * @brief Returns the device ID this volume is mounted on.
       */
      UInt32 GetDeviceID() const { return _deviceID; }

      /**
       * @brief Returns the volume label read from the BPB or root directory.
       *        Never null; falls back to "VOLUME" if no label is found.
       */
      const char* GetLabel() const { return _label; }

      /**
       * @brief Returns the total capacity of the volume in bytes.
       */
      UInt32 GetTotalBytes() const { return _totalSectors * _bytesPerSector; }

      /**
       * @brief Returns the number of free bytes on the volume by counting
       *        free clusters in the cached FAT.
       */
      UInt32 GetFreeBytes() const;

      /**
       * @brief Returns the number of used bytes on the volume.
       */
      UInt32 GetUsedBytes() const {
        return GetTotalBytes() - GetFreeBytes();
      }

      /**
       * @brief Resolves a volume-relative path to a FAT12 directory entry.
       * @param relativePath Volume-relative path (e.g. `"Dir/File.TXT"`).
       *                     An empty string or null resolves to the root.
       * @param outEntry     Receives the resolved entry.
       * @return `true` if the path was found; `false` otherwise.
       */
      bool FindEntry(const char* relativePath, ResolvedEntry* outEntry);

      /**
       * @brief Reads bytes from an open file entry.
       * @param entry Entry returned by `FindEntry` (must be a file).
       * @param offset Byte offset within the file.
       * @param size Number of bytes to read.
       * @param buffer Destination buffer.
       * @param bytesRead Receives the number of bytes actually read.
       * @return `true` on success; `false` on I/O error.
       */
      bool ReadFile(
        const ResolvedEntry& entry,
        UInt32 offset,
        UInt32 size,
        void* buffer,
        UInt32& bytesRead
      );

      /**
       * @brief Lists the entries of a directory.
       * @param relativePath Volume-relative path to the directory
       *                     (empty = root).
       * @param entries Destination array for directory entries.
       * @param maxEntries Maximum number of entries to return.
       * @param outCount Receives the number of entries written.
       * @return `true` on success; `false` on I/O error or not-a-directory.
       */
      bool ListDirectory(
        const char* relativePath,
        Servers::FileSystem::ABI::FileSystemDirectory* entries,
        UInt32 maxEntries,
        UInt32& outCount
      );

    private:
      /**
       * @brief Whether the volume has been successfully mounted.
       */
      bool _mounted = false;

      /**
       * @brief Storage device ID used for sector read requests.
       */
      UInt32 _deviceID = 0;

      /**
       * @brief Null-terminated volume label (up to 11 characters).
       */
      char _label[12] = {};

      /**
       * @brief Total number of sectors on the volume (from BPB).
       */
      UInt32 _totalSectors = 0;

      /**
       * @brief Bytes per logical sector (from BPB).
       */
      UInt32 _bytesPerSector = 512;

      /**
       * @brief Sectors per cluster (from BPB).
       */
      UInt32 _sectorsPerCluster = 1;

      /**
       * @brief LBA of the first FAT sector.
       */
      UInt32 _fatStart = 0;

      /**
       * @brief LBA of the first root directory sector.
       */
      UInt32 _rootDirectoryStart = 0;

      /**
       * @brief Number of sectors occupied by the root directory.
       */
      UInt32 _rootDirectorySectors = 0;

      /**
       * @brief LBA of the first data region sector (cluster 2).
       */
      UInt32 _dataStart = 0;

      /**
       * @brief Number of sectors per FAT copy (from BPB).
       */
      UInt32 _fatSizeSectors = 0;

      /**
       * @brief Heap-allocated copy of the first FAT.
       */
      UInt8* _fat = nullptr;

      /**
       * @brief Size of the FAT buffer in bytes.
       */
      UInt32 _fatSize = 0;

      /**
       * @brief Simple fixed-size cache for recently read sectors, using a
       *        clock algorithm for eviction. Each entry tracks the LBA it
       *        contains and whether it's valid; the data is 512 bytes.
       */
      static constexpr UInt32 SectorCacheSize = 64;

      /**
       * @brief A cached sector, storing its LBA, validity, and data.
       */
      struct CachedSector {
        /**
         * @brief LBA of the cached sector.
         */
        UInt64 LBA;

        /**
         * @brief Whether this cache entry contains valid data.
         */
        bool Valid;

        /**
         * @brief Sector data (512 bytes).
         */
        UInt8 Data[512];
      };

      /**
       * @brief The sector cache array.
       */
      CachedSector _sectorCache[SectorCacheSize] = {};

      /**
       * @brief Clock hand index for the next eviction candidate in the sector
       *        cache.
       */
      UInt32 _cacheClockHand = 0;

      /**
       * @brief Persistent shared buffer for sector reads, reused across calls
       *        to avoid repeated create/attach/destroy overhead. Must be at
       *        least 512 bytes.
       */
      Kernel::Memory::SharedBufferID _persistentBufferID = 0;

      /**
       * @brief Virtual address of the attached persistent buffer.
       */
      UIntPtr _persistentBufferVA;

      /**
       * @brief Size of the persistent buffer in bytes.
       */
      UInt32 _persistentBufferSize = 0;

      /**
       * @brief IPC handles for communicating with the Storage ABI.
       */
      Kernel::IPC::IPCPortResourceID _storageSendHandle
        = static_cast<Kernel::IPC::IPCPortResourceID>(-1);

      /**
       * @brief IPC send handle for `Storage::ABI::Read` calls.
       *
       * Created on demand and kept for the lifetime of the volume to avoid
       * repeated open/close overhead. The reply port ID is included in the
       * message payload for the Storage ABI to know where to send replies.
       */
      Kernel::IPC::IPCPortResourceID _storageReplyHandle
        = static_cast<Kernel::IPC::IPCPortResourceID>(-1);

      /**
       * @brief IPC port ID for receiving replies from the Storage ABI.
       *
       * Created on demand and kept for the lifetime of the volume to avoid
       * repeated open/close overhead. The reply port ID is included in the
       * message payload for the Storage ABI to know where to send replies.
       */
      Kernel::IPC::IPCPortID _storageReplyPortID = 0;

      /**
       * @brief Opens IPC ports for communicating with the Storage ABI.
       * 
       * The send handle is used for `Storage::ABI::Read` calls; the reply
       * handle receives the response messages. The reply port ID is included in
       * the message payload for the Storage ABI to know where to send replies.
       */
      bool _openStorageHandles();

      /**
       * @brief Closes the IPC ports used for Storage ABI communication.
       */
      void _closeStorageHandles();

      /**
       * @brief Reads a sector from the device into the cache, evicting an
       *        existing entry if necessary.
       * @param lba Logical block address of the sector to read.
       * @return Pointer to the cached sector data, or `nullptr` on I/O failure.
       */
      CachedSector* _cacheLookup(UInt64 lba);

      /**
       * @brief Evicts a sector from the cache using a clock algorithm and
       *        returns the slot for reuse.
       * @return Pointer to the evicted (now free) cache slot.
       */
      CachedSector* _cacheEvict();

      /**
       * @brief Allocates a persistent shared buffer for sector reads, if not
       *        already allocated or large enough.
       * @param sizeInBytes Required buffer size in bytes.
       * @return `true` if a suitable buffer is available; `false` on allocation
       *         failure.
       */
      bool _ensurePersistentBuffer(UInt32 sizeInBytes);

      /**
       * @brief Destroys the persistent shared buffer if it exists.
       */
      void _destroyPersistentBuffer();

      /**
       * @brief Reads `count` sectors starting at `lba` into `buffer`.
       *        Creates a temporary shared buffer for the Storage ABI call.
       * @param lba First sector to read (logical block address).
       * @param count Number of sectors to read.
       * @param buffer Destination buffer (must hold `count * bytesPerSector`).
       * @return `true` on success; `false` on allocation or I/O failure.
       */
      bool _readSectors(UInt64 lba, UInt32 count, void* buffer);

      /**
       * @brief Reads the volume label from the BPB field (offset 43) and
       *        the root directory, storing the result in `_label`.
       *        A root directory label entry takes precedence over the BPB.
       * @param bpb Sector 0 buffer (512 bytes) already read during Mount.
       */
      void _extractLabel(const UInt8* bpb);

      /**
       * @brief Returns the next cluster in a FAT12 chain.
       * @param cluster Current cluster number (≥ 2).
       * @return Next cluster number, or ≥ `ClusterEOF` at end of chain.
       */
      UInt16 _nextCluster(UInt16 cluster) const;

      /**
       * @brief Converts a cluster number (≥ 2) to an LBA sector address.
       * @param cluster The cluster number to convert.
       * @return The LBA of the first sector of the cluster.
       */
      UInt64 _clusterToLBA(UInt16 cluster) const;

      /**
       * @brief Searches the fixed root directory region for a named entry.
       * @param name Filename to search for (case-insensitive).
       * @param outEntry Receives the resolved entry on success.
       * @return `true` if found; `false` otherwise.
       */
      bool _findRootEntry(const char* name, ResolvedEntry* outEntry);

      /**
       * @brief Searches a subdirectory (given its first cluster) for a named
       *        entry.
       * @param startCluster First cluster of the subdirectory.
       * @param name Filename to search for (case-insensitive).
       * @param outEntry Receives the resolved entry on success.
       * @return `true` if found; `false` otherwise.
       */
      bool _findSubdirectoryEntry(
        UInt16 startCluster,
        const char* name,
        ResolvedEntry* outEntry
      );

      /**
       * @brief Fills directory entries from the fixed root directory region.
       * @param entries Destination array for directory entries.
       * @param maxEntries Maximum number of entries to store.
       * @param outCount Receives the number of entries written.
       * @return `true` on success; `false` on I/O error.
       */
      bool _listRootDirectory(
        Servers::FileSystem::ABI::FileSystemDirectory* entries,
        UInt32 maxEntries,
        UInt32& outCount
      );

      /**
       * @brief Fills directory entries from a subdirectory cluster chain.
       * @param startCluster First cluster of the subdirectory.
       * @param entries Destination array for directory entries.
       * @param maxEntries Maximum number of entries to store.
       * @param outCount Receives the number of entries written.
       * @return `true` on success; `false` on I/O error.
       */
      bool _listSubdirectory(
        UInt16 startCluster,
        Servers::FileSystem::ABI::FileSystemDirectory* entries,
        UInt32 maxEntries,
        UInt32& outCount
      );

      /**
       * @brief Computes the size in bytes of a subdirectory by counting the
       *        clusters in its FAT chain.
       * @param firstCluster First cluster of the subdirectory.
       * @return Size in bytes (clusters x sectors/cluster x bytes/sector).
       */
      UInt32 _directorySize(UInt16 firstCluster) const;

      /**
       * @brief Computes the size in bytes of a file by counting the clusters in
       *        its FAT chain. The `FileSize` field in the directory entry is
       *        ignored since it may be stale or zero for directories; the
       *        actual size is determined by traversing the FAT chain.
       * @param firstCluster First cluster of the file.
       * @return Size in bytes (clusters x sectors/cluster x bytes/sector).
       */
      UInt32 _contentSize(UInt16 firstCluster);

      /**
       * @brief Converts a `RawDirectoryEntry` name+ext into a human-readable
       *        null-terminated string (e.g. `"HELLO   TXT"` -> `"HELLO.TXT"`).
       * @param raw The raw directory entry to format.
       * @param outName Destination buffer (≥ 13 bytes).
       */
      static void _formatEntryName(const RawDirectoryEntry& raw, char* outName);

      /**
       * @brief Case-insensitive string equality test (ASCII only).
       * @param a First string.
       * @param b Second string.
       * @return `true` if the strings are equal ignoring case.
       */
      static bool _nameEquals(const char* a, const char* b);

      /**
       * @brief Accumulates one LFN entry into `lfnBuf` (`≥ LFNMaxLength + 1`
       *        bytes). Clears the buffer when the entry has bit 6 set
       *        (first physical LFN entry of a new name). UTF-16LE chars are
       *        narrowed to their low byte; `0x0000`/`0xFFFF` terminate the run.
       * @param lfnBuf Destination buffer for accumulated LFN characters.
       * @param lfn The raw LFN entry to process.
       */
      static void _accumulateLFN(char* lfnBuf, const RawLFNDirectoryEntry& lfn);

      /**
       * @brief Copies one `RawDirectoryEntry` into a `ResolvedEntry`, using
       *        `lfnName` as the display name when non-null and non-empty.
       * @param raw The raw directory entry.
       * @param out Receives the resolved entry.
       * @param lfnName Optional long filename (null = use 8.3 short name).
       */
      void _fillResolved(
        const RawDirectoryEntry& raw,
        ResolvedEntry* out,
        const char* lfnName = nullptr
      );

      /**
       * @brief Appends a `RawDirectoryEntry` to a `DirectoryEntry` array if it
       *        represents a visible file or directory, using `lfnName` as
       *        the display name when non-null and non-empty.
       * @param raw The raw directory entry.
       * @param lfnName Optional long filename.
       * @param entries Destination array.
       * @param maxEntries Maximum entries the array can hold.
       * @param count Current entry count; incremented on append.
       * @return `true` if the entry was appended; `false` if skipped.
       */
      bool _appendVisible(
        const RawDirectoryEntry& raw,
        const char* lfnName,
        Servers::FileSystem::ABI::FileSystemDirectory* entries,
        UInt32 maxEntries,
        UInt32& count
      );
  };
}
