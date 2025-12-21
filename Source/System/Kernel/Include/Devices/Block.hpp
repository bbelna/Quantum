/**
 * Quantum
 * (c) 2025 Brandon Belna - MIT License
 *
 * System/Kernel/Include/Devices/Block.hpp
 * Kernel block device registry and interface.
 */

#pragma once

#include <IPC.hpp>
#include <Types.hpp>

namespace Quantum::System::Kernel::Devices {
  /**
   * Block device registry and I/O interface.
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
       * Device I/O callback.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to read.
       * @param buffer
       *   Pointer to the data buffer.
       * @return
       *   True on success; false on failure.
       */
      typedef bool (*ReadCallback)(UInt32 lba, UInt32 count, void* buffer);

      /**
       * Device write callback.
       * @param lba
       *   Starting logical block address.
       * @param count
       *   Number of sectors to write.
       * @param buffer
       *   Pointer to the data buffer.
       * @return
       *   True on success; false on failure.
       */
      typedef bool (*WriteCallback)(
        UInt32 lba,
        UInt32 count,
        const void* buffer
      );

      /**
       * Registered device descriptor.
       */
      struct Device {
        /**
         * Device metadata.
         */
        Info info;

        /**
         * IPC port bound to the device (0 if unbound).
         */
        UInt32 portId;

        /**
         * Read callback for the device.
         */
        ReadCallback read;

        /**
         * Write callback for the device.
         */
        WriteCallback write;
      };

      /**
       * Device is read-only.
       */
      static constexpr UInt32 flagReadOnly = 1u << 0;

      /**
       * Device is removable media.
       */
      static constexpr UInt32 flagRemovable = 1u << 1;

      /**
       * Device is initialized and ready for I/O.
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
       * Initializes the block device registry.
       */
      static void Initialize();

      /**
       * Registers a new block device.
       * @param device
       *   Device descriptor to register.
       * @return
       *   Assigned device id, or 0 on failure.
       */
      static UInt32 Register(Device* device);

      /**
       * Unregisters a block device by id.
       * @param deviceId
       *   Identifier to remove.
       * @return
       *   True on success; false otherwise.
       */
      static bool Unregister(UInt32 deviceId);

      /**
       * Binds a block device to a driver IPC port.
       * @param deviceId
       *   Device identifier to bind.
       * @param portId
       *   IPC port id owned by the driver.
       * @return
       *   True on success; false otherwise.
       */
      static bool Bind(UInt32 deviceId, UInt32 portId);

      /**
       * Returns the number of registered block devices.
       * @return
       *   Number of devices.
       */
      static UInt32 GetCount();

      /**
       * Retrieves info for a device.
       * @param deviceId
       *   Identifier of the device to query.
       * @param outInfo
       *   Receives the device info.
       * @return
       *   True on success; false if not found.
       */
      static bool GetInfo(UInt32 deviceId, Info& outInfo);

      /**
       * Reads blocks from a device.
       * @param request
       *   Block I/O request.
       * @return
       *   True on success; false on failure.
       */
      static bool Read(const Request& request);

      /**
       * Writes blocks to a device.
       * @param request
       *   Block I/O request.
       * @return
       *   True on success; false on failure.
       */
      static bool Write(const Request& request);

    private:
      /**
       * Maximum number of registered devices.
       */
      static constexpr UInt32 _maxDevices = 8;

      /**
       * Registered device pointers.
       */
      static Device* _devices[_maxDevices];

      /**
       * Number of active devices.
       */
      static UInt32 _deviceCount;

      /**
       * Next device id to assign.
       */
      static UInt32 _nextDeviceId;

      /**
       * Finds a device by id.
       * @param deviceId
       *   Identifier to search for.
       * @return
       *   Pointer to the device, or `nullptr` if not found.
       */
      static Device* Find(UInt32 deviceId);

      /**
       * Validates a block request against the device info.
       * @param device
       *   Target device.
       * @param request
       *   Block I/O request.
       * @return
       *   True if valid; false otherwise.
       */
      static bool ValidateRequest(const Device& device, const Request& request);

      /**
       * Sends a block request to a bound driver via IPC.
       */
      static bool SendRequest(
        Device& device,
        const Request& request,
        bool write
      );

      /**
       * Copies bytes between buffers.
       */
      static void CopyBytes(void* dest, const void* src, UInt32 length);

  };
}
