/**
 * @file System/Kernel/Include/Handles.hpp
 * @brief Kernel handle table for capability-style access.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

#include "Objects/KernelObject.hpp"
#include "Objects/KernelObjectType.hpp"

namespace Quantum::System::Kernel {
  /**
   * Per-task handle table for kernel objects.
   */
  class HandleTable {
    public:
      /**
       * Handle value type.
       */
      using Handle = UInt32;

      /**
       * Maximum number of handles per task.
       */
      static constexpr UInt32 maxHandles = 64;

      /**
       * High-bit tag to distinguish handles from raw ids.
       */
      static constexpr Handle handleTag = 0x80000000u;

      /**
       * Handle table entry.
       */
      struct Entry {
        /**
         * Indicates whether the entry is in use.
         */
        bool inUse;

        /**
         * The kernel object type.
         */
        Objects::KernelObjectType type;

        /**
         * Access rights.
         */
        UInt32 rights;

        /**
         * The kernel object pointer.
         */
        Objects::KernelObject* object;

        /**
         * The handle value.
         */
        Handle handle;
      };

      /**
       * Initializes the handle table.
       */
      void Initialize();

      /**
       * Returns true if the value looks like a handle.
       */
      static bool IsHandle(Handle value);

      /**
       * Allocates a handle entry.
       * @param type
       *   Kernel object type.
       * @param object
       *   Kernel object pointer.
       * @param rights
       *   Access rights.
       * @return
       *   Handle on success; 0 on failure.
       */
      Handle Create(
        Objects::KernelObjectType type,
        Objects::KernelObject* object,
        UInt32 rights
      );

      /**
       * Closes a handle entry.
       * @param handle
       *   Handle to close.
       * @return
       *   True on success; false otherwise.
       */
      bool Close(Handle handle);

      /**
       * Duplicates a handle entry.
       * @param handle
       *   Handle to duplicate.
       * @param rights
       *   Rights mask (0 to keep original rights).
       * @return
       *   New handle on success; 0 on failure.
       */
      Handle Duplicate(Handle handle, UInt32 rights);

      /**
       * Queries a handle entry.
       * @param handle
       *   Handle to query.
       * @param outType
       *   Receives kernel object type.
       * @param outRights
       *   Receives rights mask.
       * @return
       *   True on success; false otherwise.
       */
      bool Query(
        Handle handle,
        Objects::KernelObjectType& outType,
        UInt32& outRights
      ) const;

      /**
       * Resolves a handle to an object id.
       * @param handle
       *   Handle to resolve.
       * @param type
       *   Expected kernel object type.
       * @param rights
       *   Required access rights.
       * @param outObject
       *   Output kernel object pointer.
       * @return
       *   True on success; false otherwise.
       */
      bool Resolve(
        Handle handle,
        Objects::KernelObjectType type,
        UInt32 rights,
        Objects::KernelObject*& outObject
      ) const;

    private:
      /**
       * Handle entries.
       */
      Entry _entries[maxHandles] = {};

      /**
       * Gets the index of a handle in the table.
       * @param handle
       *   Handle value.
       * @return
       *   Index in the table.
       */
      static UInt32 GetIndex(Handle handle);
  };
}
