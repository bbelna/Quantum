/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Coordinator/Include/FileSystem.hpp
 * Coordinator file system broker.
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
       * File system broker port id.
       */
      inline static UInt32 _portId = 0;

      /**
       * Registered file system services.
       */
      inline static Service _services[_maxServices] = {};

      /**
       * Finds the first registered service.
       */
      static Service* FindFirstService();

      /**
       * Finds a service by type.
       */
      static Service* FindService(ABI::FileSystem::Type type);

      /**
       * Registers or updates a file system service.
       */
      static void RegisterService(ABI::FileSystem::Type type, UInt32 portId);
  };
}
