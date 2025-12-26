/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/FileSystems/FAT12/Include/Service.hpp
 * FAT12 file system service.
 */

#pragma once

#include "Volume.hpp"

namespace Quantum::System::FileSystems::FAT12 {

  /**
   * FAT12 file system service.
   */
  class Service {
    public:
      /**
       * Entry point for the FAT12 service.
       */
      static void Main();

    private:
      /**
       * Maximum number of open handles.
       */
      static constexpr UInt32 _maxHandles = 8;

      /**
       * Base handle value for directory handles.
       */
      static constexpr ABI::FileSystem::Handle _handleBase = 0x100;

      /**
       * Directory handle state.
       */
      struct HandleState {
        /**
         * Whether the handle slot is active.
         */
        bool inUse;

        /**
         * True if this handle refers to a directory.
         */
        bool isDirectory;

        /**
         * True if this handle refers to the root directory.
         */
        bool isRoot;

        /**
         * Start cluster for directory handles.
         */
        UInt32 startCluster;

        /**
         * Next entry index to read.
         */
        UInt32 nextIndex;

        /**
         * File size in bytes.
         */
        UInt32 fileSize;

        /**
         * Current file offset in bytes.
         */
        UInt32 fileOffset;

        /**
         * Entry attribute flags.
         */
        UInt32 attributes;

        /**
         * Directory entry LBA.
         */
        UInt32 entryLBA;

        /**
         * Directory entry offset.
         */
        UInt32 entryOffset;
      };

      /**
       * Mounted FAT12 volume state.
       */
      inline static Volume* _volume = nullptr;

      /**
       * Storage for the FAT12 volume instance.
       */
      inline static Volume _volumeStorage {};

      /**
       * Initializes the FAT12 volume cache.
       */
      static void InitializeVolume();

      /**
       * Returns true if the path refers to the root directory.
       * @param path
       *   Path string to check.
       * @return
       *   True if the path is root.
       */
      static bool IsRootPath(CString path);

      /**
       * Allocates a handle slot.
       * @return
       *   Handle id or 0 on failure.
       */
      static ABI::FileSystem::Handle AllocateHandle(
        bool isDirectory,
        bool isRoot,
        UInt32 startCluster,
        UInt32 fileSize
      );

      /**
       * Releases an open handle slot.
       * @param handle
       *   Handle to release.
       */
      static void ReleaseHandle(ABI::FileSystem::Handle handle);

      /**
       * Gets the handle slot by id.
       * @param handle
       *   Handle to lookup.
       * @return
       *   Handle slot pointer or nullptr.
       */
      static HandleState* GetHandleState(ABI::FileSystem::Handle handle);

      /**
       * Resolves the parent directory for a path.
       * @param path
       *   Path string to parse.
       * @param parentCluster
       *   Receives the parent directory cluster.
       * @param parentIsRoot
       *   True if the parent is the root directory.
       * @param name
       *   Receives the final segment name.
       * @param nameBytes
       *   Size of the name buffer.
       * @return
       *   True if the parent was resolved.
       */
      static bool ResolveParent(
        CString path,
        UInt32& parentCluster,
        bool& parentIsRoot,
        char* name,
        UInt32 nameBytes
      );

      /**
       * Open handle slots.
       */
      inline static HandleState _handles[_maxHandles] = {};
  };
}
