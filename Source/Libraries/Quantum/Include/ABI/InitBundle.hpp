/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/InitBundle.hpp
 * User-mode INIT.BND access helpers.
 */

#pragma once

#include <ABI/SystemCall.hpp>

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
        char magic[8];
        UInt16 version;
        UInt16 entryCount;
        UInt32 tableOffset;
        UInt8 reserved[8];
      };

      /**
       * INIT.BND entry table layout.
       */
      struct Entry {
        char name[32];
        UInt8 type;
        UInt8 flags;
        UInt8 reserved[2];
        UInt32 offset;
        UInt32 size;
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
