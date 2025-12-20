/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/InitBundle.hpp
 * INIT.BND handling and launch helpers.
 */

#pragma once

#include <ABI/InitBundle.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel {
  /**
   * INIT.BND handling.
   */
  class InitBundle {
    public:
      /**
       * Maps INIT.BND into kernel and user space.
       */
      static void Initialize();

      /**
       * Task entry point to launch the coordinator from INIT.BND.
       */
      static void LaunchCoordinatorTask();

      /**
       * Retrieves the INIT.BND mapping for user space.
       * @param base
       *   Receives the user-space virtual base.
       * @param size
       *   Receives the bundle size in bytes.
       * @return
       *   True if INIT.BND is present and mapped; false otherwise.
       */
      static bool GetInfo(UInt32& base, UInt32& size);

      /**
       * Spawns a user task from INIT.BND by entry name.
       * @param name
       *   Null-terminated entry name.
       * @return
       *   Task id on success; 0 on failure.
       */
      static UInt32 SpawnTask(CString name);

    private:
      /**
       * Kernel virtual base for the INIT.BND mapping.
       */
      static constexpr UInt32 _initBundleVirtualBase = 0xC1000000;

      /**
       * User virtual base for the INIT.BND mapping.
       */
      static constexpr UInt32 _initBundleUserBase = 0x00900000;

      /**
       * User program base for spawned INIT.BND entries.
       */
      static constexpr UInt32 _userProgramBase = 0x00400000;

      /**
       * User stack top for spawned INIT.BND entries.
       */
      static constexpr UInt32 _userStackTop = 0x00800000;

      /**
       * User stack size (bytes) for spawned INIT.BND entries.
       */
      static constexpr UInt32 _userStackSize = 16 * 4096;

      /**
       * Kernel virtual base where INIT.BND is mapped; 0 if absent.
       */
      static UInt32 _initBundleMappedBase;

      /**
       * Size of INIT.BND in bytes; 0 if absent.
       */
      static UInt32 _initBundleMappedSize;

      /**
       * User virtual base where INIT.BND is mapped; 0 if absent.
       */
      static UInt32 _initBundleMappedUserBase;

      /**
       * Maps INIT.BND into kernel and user space.
       */
      static void MapInitBundle();

      /**
       * Validates the INIT.BND header magic.
       */
      static bool HasMagic(const ::Quantum::ABI::InitBundle::Header& header);

      /**
       * Retrieves the bundle entries table.
       */
      static const ::Quantum::ABI::InitBundle::Entry* GetBundleEntries(
        UInt32& entryCount
      );

      /**
       * Finds the coordinator entry in INIT.BND.
       */
      static const ::Quantum::ABI::InitBundle::Entry* FindCoordinatorEntry();

      /**
       * Compares an entry name to a target name.
       */
      static bool EntryNameMatches(
        const ::Quantum::ABI::InitBundle::Entry& entry,
        CString name
      );

      /**
       * Finds an entry by name in INIT.BND.
       */
      static const ::Quantum::ABI::InitBundle::Entry* FindEntryByName(
        CString name
      );
  };
}
