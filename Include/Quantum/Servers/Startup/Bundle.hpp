/**
 * @file Include/Quantum/Servers/Startup/Bundle.hpp
 * @brief Declaration of the startup bundle format and related structures.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */


#pragma once

#include <Quantum/Kernel.hpp>
#include <Quantum/Prelude.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::Startup {
  /**
   * @brief Base virtual address for loading startup bundle binaries.
   */
  inline constexpr UIntPtr StartupBinaryEntryPointBase = 0x10000000;

  /**
   * @brief Startup bundle header layout.
   */
  struct StartupBundleHeader {
    /**
     * @brief Bundle magic bytes.
     */
    char Magic[8];

    /**
     * @brief Bundle version.
     */
    UInt16 Version;

    /**
     * @brief Number of entries in the bundle.
     */
    UInt16 EntryCount;

    /**
     * @brief Offset to the entry table from the bundle base.
     */
    UInt32 TableOffset;

    /**
     * @brief Reserved bytes.
     */
    UInt8 Reserved[8];
  } __attribute__((packed));

  /**
   * @brief Startup bundle entry types.
   */
  enum class StartupBundleEntryType : UInt8 {
    /**
     * @brief System entry (e.g., graphics server, file server, etc.).
     */
    System = 1,

    /**
     * @brief Driver entry (e.g., PS2 driver, etc.).
     */
    Driver = 2,

    /**
     * @brief Filesystem backend entry (e.g., FAT12 driver).
     */
    FileSystem = 4,

    /**
     * @brief Application entry (e.g., shell, etc.).
     */
    Application = 3
  };

  /**
   * @brief Startup bundle entry format types.
   */
  enum class StartupBundleEntryFormatType : UInt8 {
    /**
     * @brief Raw binary data.
     */
    Raw = 1,

    /**
     * @brief ELF executable embedded in the bundle.
     */
    ELF = 2,

    /**
     * @brief ELF executable loaded from disk. The bundle payload for this
     *        entry is a null-terminated path string (e.g.
     *        `"System/Servers/Graphics"`).
     */
    DiskELF = 3
  };

  /**
   * @brief Startup bundle entry flags.
   */
  namespace StartupBundleEntryFlags {
    /**
     * @brief The entry is required and should be auto-launched at boot.
     */
    inline constexpr UInt8 Required = 0x01;
  }

  /**
   * @brief
   *   Startup bundle entry table layout.
   */
  struct StartupBundleEntry {
    /**
     * @brief Entry name (null-terminated if shorter than 32 bytes).
     */
    char Name[32];

    /**
     * @brief Entry type.
     */
    StartupBundleEntryType Type;

    /**
     * @brief Entry flags.
     */
    UInt8 Flags;

    /**
     * @brief Entry format type.
     */
    StartupBundleEntryFormatType Format;

    /**
     * @brief Entry offset in bytes from bundle base.
     */
    UInt32 Offset;

    /**
     * @brief Entry size in bytes.
     */
    UInt32 Size;

    /**
     * @brief Entry checksum.
     */
    UInt32 Checksum;
  } __attribute__((packed));

  /**
   * @brief Startup bundle layout.
   */
  struct StartupBundle {
    static StartupBundle* FromVirtualAddress(UIntPtr address) {
      auto* base = reinterpret_cast<UInt8*>(address);
      auto* header = reinterpret_cast<const StartupBundleHeader*>(base);
      auto* bundle = new StartupBundle();

      bundle->Header = header;
      bundle->Entries = reinterpret_cast<const StartupBundleEntry*>(
        base + header->TableOffset
      );

      return bundle;
    }

    /**
     * @brief Pointer to the bundle header.
     */
    const StartupBundleHeader* Header;

    /**
     * @brief Pointer to the entry table.
     */
    const StartupBundleEntry* Entries;

    /**
     * @brief Spawns a process for the given bundle entry.
     * @param entry The bundle entry to spawn.
     * @return The process ID of the spawned process, or -1 on failure.
     */
    inline ProcessID SpawnEntry(const StartupBundleEntry& entry) {
      switch (entry.Format) {
        case StartupBundleEntryFormatType::Raw: {
          // recover the bundle base address from the header pointer
          auto* bundleBase = reinterpret_cast<UInt8*>(
            const_cast<StartupBundleHeader*>(Header)
          );

          // get the first bundle entry and read its binary .entry header:
          //   [0] = entry point offset from binary start
          //   [1] = image size
          auto* binaryHeader = reinterpret_cast<UInt32*>(
            bundleBase + entry.Offset
          );

          UIntPtr sourceBase
            = reinterpret_cast<UIntPtr>(bundleBase + entry.Offset);
          UIntPtr entryPoint
            = StartupBinaryEntryPointBase
            + binaryHeader[0];

          Kernel::ABI::Process::SpawnParameters params {
            sourceBase,
            entry.Size
          };

          return Kernel::ABI::Process::Spawn(entry.Name, entryPoint, &params);
        } default: {
          return -1;
        }
      }
    }
  };
}
