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
      enum class Type : UInt32 {
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
       * Initializes the block device registry.
       */
      static void Initialize();

      /**
       * Handles a floppy controller interrupt notification.
       */
      static void HandleFloppyIRQ();

      /**
       * Allocates a DMA buffer for block device drivers.
       * @param sizeBytes
       *   Requested buffer size in bytes.
       * @param outPhysical
       *   Receives the physical address.
       * @param outVirtual
       *   Receives the user-space virtual address.
       * @param outSize
       *   Receives the allocated buffer size in bytes.
       * @return
       *   True on success; false otherwise.
       */
      static bool AllocateDMABuffer(
        UInt32 sizeBytes,
        UInt32& outPhysical,
        UInt32& outVirtual,
        UInt32& outSize
      );

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
       * Maximum number of floppy devices to register.
       */
      static constexpr UInt32 _maxFloppyDevices = 2;

      /**
       * Floppy drive index for drive A.
       */
      static constexpr UInt8 _floppyDriveAIndex = 0;

      /**
       * Floppy drive index for drive B.
       */
      static constexpr UInt8 _floppyDriveBIndex = 1;

      /**
       * Registered floppy device descriptors.
       */
      static Device _floppyDevices[_maxFloppyDevices];

      /**
       * Magic tag for boot drive stored in boot info reserved field.
       */
      static constexpr UInt32 _bootDriveMagic = 0x424F0000;

      /**
       * Default sector count for a 1.44MB floppy.
       */
      static constexpr UInt32 _defaultFloppySectorCount = 80 * 2 * 18;

      /**
       * DMA buffer virtual base for driver mappings.
       */
      static constexpr UInt32 _dmaBufferVirtualBase = 0x00600000;

      /**
       * Maximum physical address for DMA buffers.
       */
      static constexpr UInt32 _dmaMaxPhysicalAddress = 0x01000000;

      /**
       * Physical address of the DMA buffer (0 if unallocated).
       */
      static UInt32 _dmaBufferPhysical;

      /**
       * Size of the DMA buffer in bytes.
       */
      static UInt32 _dmaBufferBytes;

      /**
       * CMOS address port.
       */
      static constexpr UInt16 _cmosAddressPort = 0x70;

      /**
       * CMOS data port.
       */
      static constexpr UInt16 _cmosDataPort = 0x71;

      /**
       * CMOS register index for floppy drive types.
       */
      static constexpr UInt8 _cmosDriveTypeRegister = 0x10;

      /**
       * Finds a device by id.
       * @param deviceId
       *   Identifier to search for.
       * @return
       *   Pointer to the device, or `nullptr` if not found.
       */
      static Device* Find(UInt32 deviceId);

      /**
       * Reads a CMOS register value.
       * @param index
       *   Register index to read.
       * @return
       *   Value of the register.
       */
      static UInt8 ReadCMOSRegister(UInt8 index);

      /**
       * Maps a floppy drive type to a sector count.
       * @param driveType
       *   Drive type identifier.
       * @param sectorCount
       *   Receives the sector count.
       * @return
       *   True on success; false if unknown type.
       */
      static bool TryGetFloppySectorCount(
        UInt8 driveType,
        UInt32& sectorCount
      );

      /**
       * Extracts a drive type from the CMOS drive type register.
       * @param driveTypes
       *   CMOS drive types byte.
       * @param driveIndex
       *   Drive index (0 = A, 1 = B).
       * @return
       *   Drive type identifier.
       */
      static UInt8 GetFloppyDriveType(UInt8 driveTypes, UInt8 driveIndex);

      /**
       * Detects a floppy drive from CMOS for the given drive index.
       * @param driveTypes
       *   CMOS drive types byte.
       * @param driveIndex
       *   Drive index (0 = A, 1 = B).
       * @param driveType
       *   Receives the detected drive type.
       * @param sectorCount
       *   Receives the detected sector count.
       * @return
       *   True if a drive is present; false otherwise.
       */
      static bool DetectFloppyDrive(
        UInt8 driveTypes,
        UInt8 driveIndex,
        UInt8& driveType,
        UInt32& sectorCount
      );

      /**
       * Retrieves the BIOS boot drive from boot info.
       * @param bootDrive
       *   Receives the boot drive identifier.
       * @return
       *   True on success; false on failure.
       */
      static bool GetBootDrive(UInt8& bootDrive);

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
       * @param device
       *   Target device.
       * @param request
       *   Block I/O request.
       * @param write
       *   True for write requests; false for read.
       */
      static bool SendRequest(
        Device& device,
        const Request& request,
        bool write
      );

      /**
       * Copies bytes between buffers.
       * @param dest
       *   Destination buffer.
       * @param src
       *   Source buffer.
       * @param length
       *   Number of bytes to copy.
       */
      static void CopyBytes(void* dest, const void* src, UInt32 length);

  };
}
