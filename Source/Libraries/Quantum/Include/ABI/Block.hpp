/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * Libraries/Quantum/Include/ABI/Block.hpp
 * Block device syscall wrappers.
 */

#pragma once

#include <ABI/IPC.hpp>
#include <ABI/SystemCall.hpp>
#include <Types.hpp>

namespace Quantum::ABI::Devices {
  /**
   * Block device syscall wrappers.
   */
  class Block {
    public:
      /**
       * Block device info descriptor.
       */
      struct Info {
        /**
         * Device identifier assigned by the registry.
         */
        UInt32 id;

        /**
         * Device type identifier.
         */
        UInt32 type;

        /**
         * Size of a hardware sector in bytes.
         */
        UInt32 sectorSize;

        /**
         * Total number of addressable sectors.
         */
        UInt32 sectorCount;

        /**
         * Capability flags for this device.
         */
        UInt32 flags;
      };

      /**
       * Block I/O request descriptor.
       */
      struct Request {
        /**
         * Target device identifier.
         */
        UInt32 deviceId;

        /**
         * Starting logical block address.
         */
        UInt32 lba;

        /**
         * Number of sectors to transfer.
         */
        UInt32 count;

        /**
         * Pointer to the transfer buffer.
         */
        void* buffer;
      };

      /**
       * Returns the number of block devices.
       * @return
       *   Number of registered devices.
       */
      static UInt32 GetCount() {
        return InvokeSystemCall(SystemCall::Block_GetCount);
      }

      /**
       * Block device type identifiers.
       */
      enum Type : UInt32 {
        /**
         * Unknown or unspecified device type.
         */
        TypeUnknown = 0,

        /**
         * Floppy disk device.
         */
        TypeFloppy = 1
      };

      /**
       * Block device read only flag.
       */
      static constexpr UInt32 flagReadOnly = 1u << 0;

      /**
       * Block device removable media flag.
       */
      static constexpr UInt32 flagRemovable = 1u << 1;

      /**
       * Block device ready flag.
       */
      static constexpr UInt32 flagReady = 1u << 2;

      /**
       * Block I/O operation identifiers.
       */
      enum Operation : UInt32 {
        /**
         * Read request.
         */
        OpRead = 1,

        /**
         * Write request.
         */
        OpWrite = 2,

        /**
         * Response payload.
         */
        OpResponse = 3
      };

      /**
       * IPC message header size in bytes.
       */
      static constexpr UInt32 messageHeaderBytes = 7 * sizeof(UInt32);

      /**
       * Maximum IPC payload bytes available for data.
       */
      static constexpr UInt32 messageDataBytes
        = IPC::maxPayloadBytes - messageHeaderBytes;

      /**
       * IPC message exchanged with block device drivers.
       */
      struct Message {
        /**
         * Operation identifier.
         */
        UInt32 op;

        /**
         * Target device id.
         */
        UInt32 deviceId;

        /**
         * Starting logical block address.
         */
        UInt32 lba;

        /**
         * Number of sectors to transfer.
         */
        UInt32 count;

        /**
         * Reply port id for responses.
         */
        UInt32 replyPortId;

        /**
         * Status code (0 success, non-zero failure).
         */
        UInt32 status;

        /**
         * Data payload length in bytes.
         */
        UInt32 dataLength;

        /**
         * Payload buffer (read/write data).
         */
        UInt8 data[messageDataBytes];
      };

      /**
       * Retrieves device info.
       * @param deviceId
       *   Identifier of the device to query.
       * @param outInfo
       *   Receives the device info.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 GetInfo(UInt32 deviceId, Info& outInfo) {
        return InvokeSystemCall(
          SystemCall::Block_GetInfo,
          deviceId,
          reinterpret_cast<UInt32>(&outInfo),
          0
        );
      }

      /**
       * Reads blocks from a device.
       * @param request
       *   Block I/O request descriptor.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Read(const Request& request) {
        return InvokeSystemCall(
          SystemCall::Block_Read,
          reinterpret_cast<UInt32>(&request),
          0,
          0
        );
      }

      /**
       * Writes blocks to a device.
       * @param request
       *   Block I/O request descriptor.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Write(const Request& request) {
        return InvokeSystemCall(
          SystemCall::Block_Write,
          reinterpret_cast<UInt32>(&request),
          0,
          0
        );
      }

      /**
       * Binds a device to a driver IPC port.
       * @param deviceId
       *   Device identifier to bind.
       * @param portId
       *   IPC port owned by the driver.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 Bind(UInt32 deviceId, UInt32 portId) {
        return InvokeSystemCall(
          SystemCall::Block_Bind,
          deviceId,
          portId,
          0
        );
      }
  };
}
