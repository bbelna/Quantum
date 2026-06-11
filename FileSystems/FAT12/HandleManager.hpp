/**
 * @file FileSystems/FAT12/HandleManager.hpp
 * @brief Declares the open-file handle manager for the FAT12 server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Servers/FileSystem/ABI.hpp>
#include <Quantum/Core/Types.hpp>

#include "Volume.hpp"

namespace Quantum::FileSystems::FAT12 {
  /**
   * @brief Maximum number of concurrently open files the FAT12 server
   *        supports.
   */
  static constexpr UInt32 MaxOpenFiles = 32;

  /**
   * @brief Tracks the state of a single open file handle.
   */
  struct OpenFile {
    /**
     * @brief `true` if this slot is in use.
     */
    bool Valid = false;

    /**
     * @brief The resolved FAT12 directory entry for the open file.
     */
    ResolvedEntry Entry;
  };

  /**
   * @brief Linear handle manager for the FAT12 file system server.
   *
   * Handles are 1-based indices into the `_slots` array (`0` is always
   * invalid). The table is NOT thread-safe; accesses must be serialized by
   * the single-threaded server loop (for now).
   */
  class HandleManager {
    public:
      /**
       * @brief Allocates a handle for the given resolved entry.
       * @param entry The resolved FAT12 directory entry to open.
       * @return A valid `FileHandle` (`≥ 1`) on success, or `0` if the table is
       *         full.
       */
      Servers::FileSystem::ABI::FileHandle Open(const ResolvedEntry& entry);

      /**
       * @brief Releases a handle.
       * @param handle The handle to close. A handle of `0` or out-of-range is
       *               silently ignored.
       */
      void Close(Servers::FileSystem::ABI::FileHandle handle);

      /**
       * @brief Looks up a handle.
       * @param handle The handle to look up.
       * @return Pointer to the `OpenFile` entry, or `nullptr` if invalid.
       */
      OpenFile* Get(Servers::FileSystem::ABI::FileHandle handle);

    private:
      /**
       * @brief Array of open file slots.
       */
      OpenFile _slots[MaxOpenFiles];
  };
}
