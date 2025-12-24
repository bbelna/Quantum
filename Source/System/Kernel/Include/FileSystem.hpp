/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/FileSystem.hpp
 * Kernel filesystem service routing.
 */

#pragma once

#include <ABI/SystemCall.hpp>

#include "IPC.hpp"
#include "Types.hpp"

namespace Quantum::System::Kernel {
  /**
   * Kernel filesystem service registry and routing.
   */
  class FileSystem {
    public:
      /**
       * Filesystem type identifiers.
       */
      enum class Type : UInt32 {
        /**
         * FAT12 filesystem.
         */
        FAT12 = 1
      };

      /**
       * IPC message header size for filesystem service messages.
       */
      static constexpr UInt32 messageHeaderBytes = 7 * sizeof(UInt32);

      /**
       * IPC message data bytes for filesystem service messages.
       */
      static constexpr UInt32 messageDataBytes
        = IPC::maxPayloadBytes - messageHeaderBytes;

      /**
       * Filesystem service IPC message.
       */
      struct ServiceMessage {
        /**
         * Operation identifier.
         */
        UInt32 op;

        /**
         * Status code (0 success, non-zero failure).
         */
        UInt32 status;

        /**
         * Reply port id for responses.
         */
        UInt32 replyPortId;

        /**
         * First argument.
         */
        UInt32 arg0;

        /**
         * Second argument.
         */
        UInt32 arg1;

        /**
         * Third argument.
         */
        UInt32 arg2;

        /**
         * Payload length in bytes.
         */
        UInt32 dataLength;

        /**
         * Payload data.
         */
        UInt8 data[messageDataBytes];
      };

      /**
       * Registers a filesystem service with the kernel.
       * @param type
       *   Filesystem type identifier.
       * @param portId
       *   IPC port owned by the service.
       * @return
       *   True on success; false otherwise.
       */
      static bool RegisterService(Type type, UInt32 portId);

      /**
       * Dispatches a filesystem syscall to a registered service.
       * @param call
       *   Filesystem syscall identifier.
       * @param arg0
       *   EBX argument.
       * @param arg1
       *   ECX argument.
       * @param arg2
       *   EDX argument.
       * @return
       *   Result status (0 success, non-zero failure).
       */
      static UInt32 Dispatch(
        ABI::SystemCall call,
        UInt32 arg0,
        UInt32 arg1,
        UInt32 arg2
      );

    private:
      /**
       * Maximum number of filesystem services.
       */
      static constexpr UInt32 _maxServices = 4;

      /**
       * Registered filesystem service descriptor.
       */
      struct Service {
        /**
         * Filesystem type.
         */
        Type type;

        /**
         * IPC port id for the service.
         */
        UInt32 portId;
      };

      /**
       * Service registry.
       */
      inline static Service _services[_maxServices] = {};

      /**
       * Finds a service by type.
       */
      static Service* FindService(Type type);

      /**
       * Finds the first registered service.
       */
      static Service* FindFirstService();
  };
}
