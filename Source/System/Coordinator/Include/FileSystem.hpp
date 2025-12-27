/**
 * @file System/Coordinator/Include/Application.hpp
 * @brief Coordinator file system broker.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <ABI/FileSystem.hpp>
#include <Types.hpp>

namespace Quantum::System::Coordinator {
  /**
   * Coordinator file system broker.
   */
  class FileSystem {
    public:
      /**
       * Initializes the file system broker port.
       */
      static void Initialize();

      /**
       * Processes any pending file system requests.
       */
      static void ProcessPending();

    private:
      /**
       * File system service descriptor.
       */
      struct Service {
        ABI::FileSystem::Type type;
        UInt32 portId;
      };

      /**
       * Maximum number of registered file system services.
       */
      static constexpr UInt32 _maxServices = 4;

      /**
       * Maximum number of mapped handles.
       */
      static constexpr UInt32 _maxHandles = 64;

      /**
       * File system broker port id.
       */
      inline static UInt32 _portId = 0;

      /**
       * Registered file system services.
       */
      inline static Service _services[_maxServices] = {};

      /**
       * File system handle mapping.
       */
      struct HandleMap {
        /**
         * Whether the handle entry is active.
         */
        bool inUse;

        /**
         * True if this handle represents a volume.
         */
        bool isVolume;

        /**
         * Coordinator-visible handle.
         */
        ABI::FileSystem::Handle userHandle;

        /**
         * Service handle.
         */
        ABI::FileSystem::Handle serviceHandle;

        /**
         * Target service port.
         */
        UInt32 servicePort;
      };

      /**
       * Next coordinator handle value.
       */
      inline static ABI::FileSystem::Handle _nextHandle = 0x1000;

      /**
       * Active handle mappings.
       */
      inline static HandleMap _handles[_maxHandles] = {};

      /**
       * Finds the first registered service.
       * @return
       *   Pointer to the service, or `nullptr` if none registered.
       */
      static Service* FindFirstService();

      /**
       * Finds a service by type.
       * @param type
       *   File system type.
       * @return
       *   Pointer to the service, or `nullptr` if not found.
       */
      static Service* FindService(ABI::FileSystem::Type type);

      /**
       * Registers or updates a file system service.
       * @param type
       *   File system type.
       * @param portId
       *   Service port id.
       */
      static void RegisterService(ABI::FileSystem::Type type, UInt32 portId);

      /**
       * Allocates a coordinator handle for a service handle.
       * @param servicePort
       *   Service port id.
       * @param serviceHandle
       *   Service handle.
       * @param isVolume
       *   True if this handle represents a volume.
       * @return
       *   Coordinator-visible handle, or 0 on failure.
       */
      static ABI::FileSystem::Handle AllocateHandle(
        UInt32 servicePort,
        ABI::FileSystem::Handle serviceHandle,
        bool isVolume
      );

      /**
       * Finds a mapped handle entry.
       * @param userHandle
       *   Coordinator-visible handle.
       * @param expectVolume
       *   True if expecting a volume handle.
       * @return
       *   Pointer to the handle mapping, or `nullptr` if not found.
       */
      static HandleMap* FindHandle(
        ABI::FileSystem::Handle userHandle,
        bool expectVolume
      );

      /**
       * Releases a handle mapping.
       * @param userHandle
       *   Coordinator-visible handle.
       */
      static void ReleaseHandle(ABI::FileSystem::Handle userHandle);
  };
}
