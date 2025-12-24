/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/FileSystem.hpp
 * Kernel file system service routing.
 */

#pragma once

#include <ABI/SystemCall.hpp>

#include "IPC.hpp"
#include "Types.hpp"

namespace Quantum::System::Kernel {
  /**
   * Kernel file system service registry and routing.
   */
  class FileSystem {
    public:
      /**
       * File system type identifiers.
       */
      enum class Type : UInt32 {
        /**
         * FAT12 file system.
         */
        FAT12 = 1
      };

      /**
       * IPC message header size for file system service messages.
       */
      static constexpr UInt32 messageHeaderBytes = 7 * sizeof(UInt32);

      /**
       * IPC message data bytes for file system service messages.
       */
      static constexpr UInt32 messageDataBytes
        = IPC::maxPayloadBytes - messageHeaderBytes;

      /**
       * File system service IPC message.
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
       * Volume entry descriptor.
       */
      struct VolumeEntry {
        /**
         * Volume label (null-terminated).
         */
        char label[16];

        /**
         * File system type identifier.
         */
        UInt32 fsType;
      };

      /**
       * Volume information descriptor.
       */
      struct VolumeInfo {
        /**
         * Volume label (null-terminated).
         */
        char label[16];

        /**
         * File system type identifier.
         */
        UInt32 fsType;

        /**
         * Bytes per sector.
         */
        UInt32 sectorSize;

        /**
         * Total sector count.
         */
        UInt32 sectorCount;

        /**
         * Free sector count.
         */
        UInt32 freeSectors;
      };

      /**
       * Registers a file system service with the kernel.
       * @param type
       *   File system type identifier.
       * @param portId
       *   IPC port owned by the service.
       * @return
       *   True on success; false otherwise.
       */
      static bool RegisterService(Type type, UInt32 portId);

      /**
       * Dispatches a file system syscall to a registered service.
       * @param call
       *   File system syscall identifier.
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
       * Maximum number of file system services.
       */
      static constexpr UInt32 _maxServices = 4;

      /**
       * Registered file system service descriptor.
       */
      struct Service {
        /**
         * File system type.
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
