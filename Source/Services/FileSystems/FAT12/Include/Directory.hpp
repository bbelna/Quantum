/**
 * @file Services/FileSystems/FAT12/Include/Directory.hpp
 * @brief FAT12 file system directory helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/FileSystem.hpp>
#include <ABI/Prelude.hpp>
#include <Types.hpp>

namespace Quantum::Services::FileSystems::FAT12 {
  class Volume;

  /**
   * FAT12 directory traversal helpers.
   */
  class Directory {
    public:
      /**
       * Directory record descriptor.
       */
      struct Record {
        /**
         * Entry name in 8.3 format.
         */
        UInt8 name[11];

        /**
         * Entry long name.
         */
        char longName[ABI::FileSystem::maxDirectoryLength];

        /**
         * Entry attribute flags.
         */
        UInt8 attributes;

        /**
         * FAT create time.
         */
        UInt16 createTime;

        /**
         * FAT create date.
         */
        UInt16 createDate;

        /**
         * FAT last access date.
         */
        UInt16 accessDate;

        /**
         * FAT last write time.
         */
        UInt16 writeTime;

        /**
         * FAT last write date.
         */
        UInt16 writeDate;

        /**
         * Entry start cluster.
         */
        UInt16 startCluster;

        /**
         * Entry size in bytes.
         */
        UInt32 sizeBytes;
      };

      /**
       * Initializes with a volume.
       * @param volume
       *   Volume to access.
       */
      void Initialize(Volume& volume);

      /**
       * Reads a root directory entry by index.
       * @param index
       *   Root directory entry index.
       * @param record
       *   Output record.
       * @param end
       *   True if end of directory reached.
       * @return
       *   True on success.
       */
      bool ReadRootRecord(UInt32 index, Record& record, bool& end);

      /**
       * Reads a directory record by index from a cluster chain.
       * @param startCluster
       *   First cluster of the directory.
       * @param index
       *   Record index.
       * @param record
       *   Output record.
       * @param end
       *   True if end of directory reached.
       * @return
       *   True on success.
       */
      bool ReadRecord(
        UInt32 startCluster,
        UInt32 index,
        Record& record,
        bool& end
      );

      /**
       * Reads a directory record from a known location.
       * @param lba
       *   Entry sector LBA.
       * @param offset
       *   Entry offset within the sector.
       * @param record
       *   Output record.
       * @return
       *   True on success.
       */
      bool ReadRecordAt(UInt32 lba, UInt32 offset, Record& record);

      /**
       * Converts a directory record into a directory entry.
       * @param record
       *   Record to convert.
       * @param entry
       *   Output entry.
       * @return
       *   True on success.
       */
      static bool RecordToEntry(
        const Record& record,
        ABI::FileSystem::DirectoryEntry& entry
      );

      /**
       * Builds an 8.3 short name.
       * @param name
       *   Null-terminated name.
       * @param outName
       *   Receives 8.3 name bytes.
       * @return
       *   True on success.
       */
      static bool BuildShortName(CString name, UInt8* outName);

      /**
       * Finds a directory entry by name.
       * @param startCluster
       *   Directory cluster.
       * @param isRoot
       *   True if root directory.
       * @param name
       *   Entry name.
       * @param outCluster
       *   Receives start cluster.
       * @param outAttributes
       *   Receives attributes.
       * @param outSize
       *   Receives size in bytes.
       * @return
       *   True if found.
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
       * Finds a directory entry and its location.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Entry name.
       * @param record
       *   Receives record.
       * @param outLBA
       *   Receives sector LBA.
       * @param outOffset
       *   Receives byte offset within sector.
       * @return
       *   True if found.
       */
      bool FindEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        Record& record,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Updates a directory entry at a known location.
       * @param lba
       *   Directory sector LBA.
       * @param offset
       *   Entry byte offset within sector.
       * @param startCluster
       *   Updated start cluster.
       * @param sizeBytes
       *   Updated size.
       * @return
       *   True on success.
       */
      bool UpdateEntry(
        UInt32 lba,
        UInt32 offset,
        UInt16 startCluster,
        UInt32 sizeBytes
      );

      /**
       * Returns true if a directory has no user entries.
       * @param startCluster
       *   Directory cluster.
       * @return
       *   True if empty.
       */
      bool IsEmpty(UInt32 startCluster);

      /**
       * Returns true if the record is "." or "..".
       * @param record
       *   Directory record.
       * @return
       *   True if dot record.
       */
      static bool IsDotRecord(const Record& record);

      /**
       * Creates a directory entry and allocates its cluster.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Directory name.
       * @return
       *   True on success.
       */
      bool CreateDirectory(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name
      );

      /**
       * Creates a file entry.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   File name.
       * @return
       *   True on success.
       */
      bool CreateFile(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name
      );

      /**
       * Removes a directory entry.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Entry name.
       * @return
       *   True on success.
       */
      bool RemoveEntry(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name
      );

      /**
       * Renames a directory entry.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Existing entry name.
       * @param newName
       *   New entry name.
       * @return
       *   True on success.
       */
      bool RenameEntry(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        CString newName
      );

      /**
       * Returns entry metadata for a path.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Entry name.
       * @param outInfo
       *   Receives file info.
       * @param outAttributes
       *   Receives attributes.
       * @return
       *   True on success.
       */
      bool GetEntryInfo(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        ABI::FileSystem::FileInfo& outInfo,
        UInt8& outAttributes
      );

    private:
      /**
       * Long filename tracking state.
       */
      struct LFNState {
        char name[ABI::FileSystem::maxDirectoryLength];
        UInt8 checksum;
        UInt8 expected;
        UInt8 seenMask;
        bool active;
      };

      /**
       * Clears LFN tracking state.
       * @param state
       *   State to clear.
       */
      static void ClearLFN(LFNState& state);

      /**
       * Computes the LFN checksum for a short name.
       * @param shortName
       *   8.3 name bytes.
       * @return
       *   LFN checksum.
       */
      static UInt8 LFNChecksum(const UInt8* shortName);

      /**
       * Copies UTF-16 LFN characters into the state buffer.
       * @param state
       *   LFN state to update.
       * @param offset
       *   Output offset.
       * @param base
       *   Raw LFN entry data pointer.
       * @param count
       *   Character count.
       */
      static void CopyLFNChars(
        LFNState& state,
        UInt32 offset,
        const UInt8* base,
        UInt32 count
      );

      /**
       * Parses an LFN entry into the tracking state.
       * @param base
       *   Raw entry pointer.
       * @param state
       *   LFN state to update.
       */
      static void ParseLFNEntry(const UInt8* base, LFNState& state);

      /**
       * Returns true if the LFN state matches the short name.
       * @param state
       *   LFN state.
       * @param shortName
       *   8.3 name bytes.
       * @return
       *   True if the LFN should be used.
       */
      static bool UseLFN(const LFNState& state, const UInt8* shortName);

      /**
       * Populates a record from a raw entry.
       * @param volume
       *   Volume instance.
       * @param base
       *   Raw entry pointer.
       * @param lfn
       *   LFN state.
       * @param record
       *   Record to fill.
       */
      static void PopulateRecord(
        Volume* volume,
        const UInt8* base,
        const LFNState& lfn,
        Record& record
      );

      /**
       * Writes fixed FAT timestamps into an entry.
       * @param volume
       *   Volume instance.
       * @param entryBytes
       *   Raw entry bytes.
       * @param setCreate
       *   True to set create timestamp.
       * @param setAccess
       *   True to set access date.
       * @param setWrite
       *   True to set write timestamp.
       */
      static void WriteTimestamps(
        Volume* volume,
        UInt8* entryBytes,
        bool setCreate,
        bool setAccess,
        bool setWrite
      );

      /**
       * Builds an 8.3 alias for long names.
       * @param name
       *   Long file name.
       * @param outName
       *   Receives 8.3 alias.
       * @return
       *   True on success.
       */
      static bool BuildShortAlias(CString name, UInt8* outName);

      /**
       * Finds a contiguous run of free directory slots.
       * @param startCluster
       *   Directory cluster.
       * @param isRoot
       *   True if root directory.
       * @param count
       *   Number of entries needed.
       * @param outIndex
       *   Receives starting entry index.
       * @return
       *   True on success.
       */
      bool FindFreeSlotRun(
        UInt32 startCluster,
        bool isRoot,
        UInt32 count,
        UInt32& outIndex
      );

      /**
       * Computes the on-disk location for an entry index.
       * @param parentCluster
       *   Directory cluster.
       * @param parentIsRoot
       *   True if root directory.
       * @param entryIndex
       *   Entry index.
       * @param outLBA
       *   Receives entry LBA.
       * @param outOffset
       *   Receives entry offset.
       * @return
       *   True on success.
       */
      bool ComputeEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        UInt32 entryIndex,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Writes LFN entries for a long name.
       * @param parentCluster
       *   Directory cluster.
       * @param parentIsRoot
       *   True if root directory.
       * @param entryIndex
       *   First entry index to use.
       * @param name
       *   Long file name.
       * @param shortName
       *   Short name bytes.
       * @return
       *   True on success.
       */
      bool WriteLFNEntries(
        UInt32 parentCluster,
        bool parentIsRoot,
        UInt32 entryIndex,
        CString name,
        const UInt8* shortName
      );
      /**
       * Writes a raw directory entry.
       * @param lba
       *   Directory sector LBA.
       * @param offset
       *   Entry byte offset within sector.
       * @param entryBytes
       *   Raw entry bytes.
       * @return
       *   True on success.
       */
      bool WriteEntry(
        UInt32 lba,
        UInt32 offset,
        const UInt8* entryBytes
      );

      /**
       * Finds the next free directory entry slot.
       * @param startCluster
       *   Directory cluster.
       * @param isRoot
       *   True if root directory.
       * @param outLBA
       *   Receives sector LBA.
       * @param outOffset
       *   Receives byte offset.
       * @return
       *   True on success.
       */
      bool FindFreeSlot(
        UInt32 startCluster,
        bool isRoot,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Associated volume.
       */
      Volume* _volume;
  };
}
