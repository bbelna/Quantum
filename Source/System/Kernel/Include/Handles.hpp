/**
 * @file System/Kernel/Include/Handles.hpp
 * @brief Kernel handle table for capability-style access.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright Ac 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <Types.hpp>

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
       * Handle entry object types.
       */
      enum class ObjectType : UInt32 {
        /**
         * No object.
         */
        None = 0,

        /**
         * IPC port object.
         */
        IpcPort = 1
      };

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
         * The object type.
         */
        ObjectType type;

        /**
         * Access rights.
         */
        UInt32 rights;

        /**
         * The object id.
         */
        UInt32 objectId;

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
       *   Object type.
       * @param objectId
       *   Object id.
       * @param rights
       *   Access rights.
       * @return
       *   Handle on success; 0 on failure.
       */
      Handle Create(ObjectType type, UInt32 objectId, UInt32 rights);

      /**
       * Closes a handle entry.
       * @param handle
       *   Handle to close.
       * @return
       *   True on success; false otherwise.
       */
      bool Close(Handle handle);

      /**
       * Resolves a handle to an object id.
       * @param handle
       *   Handle to resolve.
       * @param type
       *   Expected object type.
       * @param rights
       *   Required access rights.
       * @param outObjectId
       *   Output object id.
       * @return
       *   True on success; false otherwise.
       */
      bool Resolve(
        Handle handle,
        ObjectType type,
        UInt32 rights,
        UInt32& outObjectId
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
