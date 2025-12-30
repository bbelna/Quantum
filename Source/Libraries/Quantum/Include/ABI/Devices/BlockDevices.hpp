/**
 * @file Libraries/Quantum/Include/ABI/Devices/BlockDevices.hpp
 * @brief Block device syscall wrappers.
 * @author Brandon Belna <bbelna@aol.com>
 * @copyright © 2025-2026 The Quantum OS Project
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include "ABI/IPC.hpp"
#include "ABI/SystemCall.hpp"
#include "Types.hpp"

namespace Quantum::ABI::Devices {
  /**
   * Block device syscall wrappers.
   */
  class BlockDevices {
    public:
      /**
       * Block device handle type.
       */
      using Handle = UInt32;
      /**
       * Block I/O operation identifiers.
       */
      enum class Operation : UInt32 {
        /**
         * Read request.
         */
        Read = 1,

        /**
         * Write request.
         */
        Write = 2,

        /**
         * Response payload.
         */
        Response = 3
      };

      /**
       * Block device type identifiers.
       */
      enum  class Type : UInt32 {
        /**
         * Unknown or unspecified device type.
         */
        Unknown = 0,

        /**
         * Floppy disk device.
         */
        Floppy = 1
      };

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
        Type type;

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

        /**
         * Controller-specific device index (e.g., floppy A=0, B=1).
         */
        UInt32 deviceIndex;
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
        Operation op;

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
       * DMA buffer descriptor.
       */
      struct DMABuffer {
        /**
         * Physical address of the DMA buffer.
         */
        UInt32 physical;

        /**
         * Virtual address of the DMA buffer.
         */
        void* virtualAddress;

        /**
         * Size of the DMA buffer in bytes.
         */
        UInt32 size;
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
       * Block device read right.
       */
      static constexpr UInt32 RightRead = 1u << 0;

      /**
       * Block device write right.
       */
      static constexpr UInt32 RightWrite = 1u << 1;

      /**
       * Block device control right.
       */
      static constexpr UInt32 RightControl = 1u << 2;

      /**
       * Block device bind right.
       */
      static constexpr UInt32 RightBind = 1u << 3;

      /**
       * Maximum sector size supported by WritePartial.
       */
      static constexpr UInt32 helperMaxBytes = 4096;

      /**
       * Returns the number of block devices.
       * @return
       *   Number of registered devices.
       */
      static UInt32 GetCount() {
        return InvokeSystemCall(SystemCall::Block_GetCount);
      }

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
       * Opens a handle to a block device.
       * @param deviceId
       *   Device identifier.
       * @param rights
       *   Rights mask.
       * @return
       *   Handle on success; 0 on failure.
       */
      static Handle Open(UInt32 deviceId, UInt32 rights) {
        return InvokeSystemCall(SystemCall::Block_Open, deviceId, rights, 0);
      }

      /**
       * Updates device info for a bound device.
       * @param deviceId
       *   Device identifier to update.
       * @param info
       *   Updated info payload (id and type must match).
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 UpdateInfo(UInt32 deviceId, const Info& info) {
        return InvokeSystemCall(
          SystemCall::Block_UpdateInfo,
          deviceId,
          reinterpret_cast<UInt32>(&info),
          0
        );
      }

      /**
       * Registers a new block device with the kernel registry.
       * @param info
       *   Device info payload (id ignored).
       * @return
       *   Assigned device id on success, 0 on failure.
       */
      static UInt32 Register(const Info& info) {
        return InvokeSystemCall(
          SystemCall::Block_Register,
          reinterpret_cast<UInt32>(&info),
          0,
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
       * Writes a byte range within a single sector.
       * @param deviceId
       *   Device identifier.
       * @param lba
       *   Target logical block address.
       * @param offsetBytes
       *   Byte offset within the sector.
       * @param data
       *   Source data to write.
       * @param lengthBytes
       *   Number of bytes to write.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 WritePartial(
        UInt32 deviceId,
        UInt32 lba,
        UInt32 offsetBytes,
        const void* data,
        UInt32 lengthBytes
      ) {
        if (!data || lengthBytes == 0) {
          return 1;
        }

        Info info {};

        if (GetInfo(deviceId, info) != 0) {
          return 2;
        }

        UInt32 sectorSize = info.sectorSize;

        if (sectorSize == 0 || sectorSize > helperMaxBytes) {
          return 3;
        }

        if (offsetBytes + lengthBytes > sectorSize) {
          return 4;
        }

        UInt8 buffer[helperMaxBytes] = {};
        Request request {};

        request.deviceId = deviceId;
        request.lba = lba;
        request.count = 1;
        request.buffer = buffer;

        if (Read(request) != 0) {
          return 5;
        }

        const UInt8* src = reinterpret_cast<const UInt8*>(data);

        for (UInt32 i = 0; i < lengthBytes; ++i) {
          buffer[offsetBytes + i] = src[i];
        }

        return Write(request);
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

      /**
       * Allocates a DMA buffer and maps it into the caller's address space.
       * @param sizeBytes
       *   Requested buffer size in bytes.
       * @param outBuffer
       *   Receives the DMA buffer descriptor.
       * @return
       *   0 on success, non-zero on failure.
       */
      static UInt32 AllocateDMABuffer(
        UInt32 sizeBytes,
        DMABuffer& outBuffer
      ) {
        return InvokeSystemCall(
          SystemCall::Block_AllocateDMABuffer,
          sizeBytes,
          reinterpret_cast<UInt32>(&outBuffer),
          0
        );
      }
  };
}
