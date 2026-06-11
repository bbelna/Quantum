/**
 * @file Bootloader/Platform/PC/FAT12FileLoader.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::FAT12FileLoader.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Platform/PC/PCTypes.hpp>

namespace Quantum::Bootloader::Platform::PC {
  /**
   * @brief FAT12 implementation of @ref IFileLoader.
   *
   * Loads files from a FAT12 file system using the bootloader's BIOS-based
   * disk I/O routines. Supports paths with a single directory level
   * (e.g., `"/System/Kernel.qbn"`).
   */
  class FAT12FileLoader : public IFileLoader {
    public:
      /**
       * @brief Maximum number of path components (directory depth + filename).
       */
      static constexpr Size MaxPathComponents = 4;

      /**
       * @brief Initializes FAT12 state using the current boot drive.
       * @param bootDrive BIOS drive number (e.g. `0x00` for floppy A).
       */
      void Initialize(UInt8 bootDrive);

      /**
       * @brief Sets the spinner used while file sectors are loading.
       * @param spinner Pointer to the spinner instance, or `nullptr`.
       */
      void SetSpinner(ISpinner* spinner) override {
        _spinner = spinner;
      }

      /**
       * @brief Loads a file from the specified path into memory.
       * @param path The file path to load (e.g., `"/System/Kernel.qbn"`).
       * @param address Physical address to load the file at.
       * @return `true` on success; `false` on failure.
       */
      bool Load(
        const char* path,
        UInt32 address,
        UInt32* outLoadedSize
      ) override;

    private:
      /**
       * @brief Descriptor for a file found in FAT12.
       */
      struct FileInfo {
        UInt16 FirstCluster;
        UInt32 Size;
        UInt16 Sectors;
      };

      /**
       * @brief Highest physical address reachable by real-mode segment:offset
       *        addressing.
       *
       * BIOS `INT 13h` uses `ES:BX` to specify the read destination, which
       * limits the target to the first megabyte of memory.  Destinations at
       * or above this address must go through @ref _bounceBuffer.
       */
      static constexpr UInt32 _realModeAddressLimit = 0x00100000;

      /**
       * @brief Low-memory scratch buffer for bouncing sector reads.
       *
       * When a sector's destination lies above @ref _realModeAddressLimit,
       * `_readSector` reads the sector here first, then copies it to the
       * actual target in protected mode.  Must be large enough to hold one
       * sector (512 bytes for FAT12 floppy media).
       */
      static constexpr UInt32 _bounceBuffer = 0x1000;

      /**
       * @brief Low-memory buffer for the INT 13h extensions Disk Address
       *        Packet (DAP). Must be at a fixed real-mode-accessible address.
       */
      static constexpr UInt32 _dapBuffer = 0x0F00;

      /**
       * @brief Physical address of stage 1's BIOS parameter block.
       */
      static constexpr UInt32 _bpbBase = 0x7C00;

      /**
       * @brief Scratch buffer for root directory sector reads.
       */
      static constexpr UInt32 _rootDirectoryBuffer = 0x2000;

      /**
       * @brief Physical address of the FAT table loaded by stage 2.
       */
      static constexpr UInt32 _fatBuffer = 0x3000;

      /**
       * @brief Converts a single path component to an 11-byte FAT12 8.3 name.
       * @param component Null-terminated or `/`-terminated component string.
       * @param length Length of the component (excluding terminator).
       * @param out 11-byte output buffer, space-padded.
       */
      static void _toFAT12Name(const char* component, Size length, char* out);

      /**
       * @brief Splits a path into components.
       * @param path Null-terminated path (e.g., `"/System/Kernel.qbn"`).
       * @param components Output array of component start pointers.
       * @param lengths Output array of component lengths.
       * @return Number of components parsed.
       */
      static Size _splitPath(
        const char* path,
        const char* components[MaxPathComponents],
        Size lengths[MaxPathComponents]
      );

      bool _readSector(UInt16 lba, UInt32 destinationPhysicalAddress) const;
      UInt16 _getNextCluster(UInt16 cluster) const;
      UInt16 _clusterToLBA(UInt16 cluster) const;
      bool _memCompare(const void* a, const void* b, UInt32 n) const;
      bool _findFile(const char* name, FileInfo* out) const;
      bool _findDirectory(const char* name, UInt16* outFirstCluster) const;
      bool _findFileInDirectory(
        const char* name,
        UInt16 dirFirstCluster,
        FileInfo* out
      ) const;
      bool _loadFile(
        const FileInfo& file,
        UInt32 destinationPhysicalAddress
      ) const;

      ISpinner* _spinner = nullptr;
      UInt8 _bootDrive = 0;
      UInt16 _bytesPerSector = 0;
      UInt8 _sectorsPerCluster = 0;
      UInt16 _reservedSectors = 0;
      UInt8 _numberOfFATs = 0;
      UInt16 _rootEntries = 0;
      UInt16 _sectorsPerFAT = 0;
      UInt16 _sectorsPerTrack = 0;
      UInt16 _numberOfHeads = 0;
      UInt16 _rootDirectorySectors = 0;
      UInt16 _firstRootSector = 0;
      UInt16 _firstDataSector = 0;
      bool _initialized = false;
  };
}
