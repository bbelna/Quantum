/**
 * @file Libraries/Quantum/Include/ABI/InitBundle.hpp
 * @brief User-mode INIT.BND access helpers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */


#pragma once

#include "ABI/SystemCall.hpp"

namespace Quantum::ABI {
  /**
   * INIT bundle ABI helpers.
   */
  class InitBundle {
    public:
      /**
       * INIT.BND header layout.
       */
      struct Header {
        /**
         * Bundle magic bytes.
         */
        char magic[8];

        /**
         * Bundle version.
         */
        UInt16 version;

        /**
         * Number of entries in the bundle.
         */
        UInt16 entryCount;

        /**
         * Offset to the entry table from the bundle base.
         */
        UInt32 tableOffset;

        /**
         * Reserved bytes.
         */
        UInt8 reserved[8];
      };

      /**
       * INIT.BND entry types.
       */
      enum class EntryType : UInt8 {
        Init = 1,
        Driver = 2,
        Service = 3
      };

      /**
       * INIT.BND entry table layout.
       */
      struct Entry {
        /**
         * Entry name (null-terminated if shorter than 32 bytes).
         */
        char name[32];

        /**
         * Entry type.
         */
        EntryType type;

        /**
         * Entry flags.
         */
        UInt8 flags;

        /**
         * Device type identifier (0 if not device-bound).
         */
        UInt8 device;

        /**
         * Device dependency mask (bitset of required devices).
         */
        UInt8 dependsMask;

        /**
         * Entry offset in bytes from bundle base.
         */
        UInt32 offset;

        /**
         * Entry size in bytes.
         */
        UInt32 size;

        /**
         * Entry checksum.
         */
        UInt32 checksum;
      };

      /**
       * INIT.BND bundle info.
       */
      struct Info {
        /**
         * Virtual address of the bundle mapping in user space.
         */
        UInt32 base;

        /**
         * Size of the bundle in bytes.
         */
        UInt32 size;
      };

      /**
       * Retrieves INIT.BND bundle info from the kernel.
       * @param out
       *   Output structure populated by the kernel.
       * @return
       *   True if the bundle exists; false if not available.
       */
      static bool GetInfo(Info& out) {
        UInt32 result = InvokeSystemCall(
          SystemCall::InitBundle_GetInfo,
          reinterpret_cast<UInt32>(&out)
        );

        return result == 0;
      }

      /**
       * Spawns a task from an INIT.BND entry by name.
       * @param name
       *   Null-terminated entry name.
       * @return
       *   Task id on success; 0 on failure.
       */
      static UInt32 Spawn(CString name) {
        return InvokeSystemCall(
          SystemCall::InitBundle_SpawnTask,
          reinterpret_cast<UInt32>(name)
        );
      }
  };
}
