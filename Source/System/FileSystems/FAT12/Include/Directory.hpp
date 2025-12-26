/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Include/Directory.hpp
 * FAT12 directory helpers.
 */

#pragma once

#include <ABI/FileSystem.hpp>
#include <ABI/Prelude.hpp>
#include <Types.hpp>

namespace Quantum::System::FileSystems::FAT12 {
  class Volume;

  /**
   * FAT12 directory traversal helpers.
   */
  class Directory {
    public:
      /**
       * Initializes the helper with a volume.
       * @param volume
       *   Volume to access.
       */
      void Init(Volume& volume);

      /**
       * Directory record descriptor.
       */
      struct DirectoryRecord {
        /**
         * Entry name in 8.3 format.
         */
        UInt8 name[11];

        /**
         * Entry attribute flags.
         */
        UInt8 attributes;

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
      bool ReadRootRecord(UInt32 index, DirectoryRecord& record, bool& end);

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
      bool ReadDirectoryRecord(
        UInt32 startCluster,
        UInt32 index,
        DirectoryRecord& record,
        bool& end
      );

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
        const DirectoryRecord& record,
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
        DirectoryRecord& record,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Finds a directory entry location without returning data.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Entry name.
       * @param outLBA
       *   Receives sector LBA.
       * @param outOffset
       *   Receives byte offset within sector.
       * @return
       *   True if found.
       */
      bool GetDirectoryEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
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
      bool UpdateDirectoryEntry(
        UInt32 lba,
        UInt32 offset,
        UInt16 startCluster,
        UInt32 sizeBytes
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
      bool WriteDirectoryEntry(
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
      bool FindFreeDirectorySlot(
        UInt32 startCluster,
        bool isRoot,
        UInt32& outLBA,
        UInt32& outOffset
      );

      /**
       * Marks a directory entry deleted.
       * @param lba
       *   Entry sector LBA.
       * @param offset
       *   Entry byte offset.
       * @return
       *   True on success.
       */
      bool MarkEntryDeleted(UInt32 lba, UInt32 offset);

      /**
       * Renames a directory entry at a known location.
       * @param lba
       *   Entry sector LBA.
       * @param offset
       *   Entry byte offset.
       * @param shortName
       *   New 8.3 name bytes.
       * @return
       *   True on success.
       */
      bool RenameDirectoryEntry(
        UInt32 lba,
        UInt32 offset,
        const UInt8* shortName
      );

      /**
       * Returns true if a directory has no user entries.
       * @param startCluster
       *   Directory cluster.
       * @return
       *   True if empty.
       */
      bool IsDirectoryEmpty(UInt32 startCluster);

      /**
       * Returns true if the record is "." or "..".
       * @param record
       *   Directory record.
       * @return
       *   True if dot record.
       */
      static bool IsDotRecord(const DirectoryRecord& record);

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
       * @param outAttributes
       *   Receives attributes.
       * @param outSize
       *   Receives size.
       * @return
       *   True on success.
       */
      bool GetEntryInfo(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        UInt8& outAttributes,
        UInt32& outSize
      );

      /**
       * Returns entry location for a path.
       * @param parentCluster
       *   Parent directory cluster.
       * @param parentIsRoot
       *   True if parent is root.
       * @param name
       *   Entry name.
       * @param outLBA
       *   Receives entry LBA.
       * @param outOffset
       *   Receives entry offset.
       * @return
       *   True on success.
       */
      bool GetEntryLocation(
        UInt32 parentCluster,
        bool parentIsRoot,
        CString name,
        UInt32& outLBA,
        UInt32& outOffset
      );

    private:
      /**
       * Associated volume.
       */
      Volume* _volume;
  };
}
