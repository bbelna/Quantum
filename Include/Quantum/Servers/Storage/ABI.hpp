/**
 * @file Include/Quantum/Servers/Storage/ABI.hpp
 * @brief Declaration of the storage server's ABI namespace.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Clients/KernelClient.hpp>
#include <Quantum/ABI.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Threading.hpp>

namespace Quantum::Servers::Storage::ABI {
  /**
   * @brief ABI version for the storage server.
   */
  constexpr UInt32 Version = 1;

  /**
   * @brief IPC port ID for the storage server.
   */
  constexpr IPCPortID PortID = 8;

  /**
   * @brief Maximum number of storage devices the server can manage at once.
   */
  constexpr UInt32 MaxDevices = 16;

  /**
   * @brief Operations supported by the storage server.
   */
  enum class StorageOperation : UInt32 {
    /**
     * @brief Returns descriptors for all registered block devices.
     */
    GetDevices = 1,

    /**
     * @brief Returns the descriptor for a single device by ID.
     */
    GetDeviceInfo = 2,

    /**
     * @brief Reads one or more sectors from a block device into a
     *        caller-allocated shared memory buffer.
     */
    Read = 3,

    /**
     * @brief Writes one or more sectors to a block device from a
     *        caller-allocated shared memory buffer.
     */
    Write = 4,

    /**
     * @brief Flushes any write-back cache for a device, ensuring all
     *        pending writes have reached the physical medium.
     */
    Flush = 5,

    /**
     * @brief Queries the media presence state of a removable device (e.g.
     *        floppy, optical). For non-removable devices always returns
     *        `Error::None`.
     */
    GetMediaStatus = 6,

    /**
     * @brief Registers a hardware storage driver with the server. Sent by
     *        driver processes during initialization with the IPC port ID on
     *        which the Storage server can send I/O requests back, plus
     *        descriptors for each device the driver manages.
     * @note Fire-and-forget, no reply is sent. Drivers must not wait for a
     *       response after sending this message.
     */
    RegisterDriver = 7
  };

  /**
   * @brief Error codes returned by storage server operations.
   */
  enum class StorageOperationErrorCode : UInt32 {
    /**
     * @brief The operation completed successfully.
     */
    None = 0,

    /**
     * @brief The specified device ID does not correspond to any registered
     *        device.
     */
    InvalidDevice = 1,

    /**
     * @brief The requested LBA is beyond the end of the device.
     */
    InvalidLBA = 2,

    /**
     * @brief `SectorCount` is zero or would cause the transfer to exceed the
     *        end of the device.
     */
    InvalidCount = 3,

    /**
     * @brief The hardware reported a read failure.
     */
    ReadError = 4,

    /**
     * @brief The hardware reported a write failure.
     */
    WriteError = 5,

    /**
     * @brief The device exists but is not ready (e.g. floppy motor not yet
     *        at speed, or drive reset in progress).
     */
    DeviceNotReady = 6,

    /**
     * @brief A write was attempted on a read-only or write-protected device.
     */
    WriteProtected = 7,

    /**
     * @brief The hardware did not respond within the expected time.
     */
    Timeout = 8,

    /**
     * @brief Removable media was changed since the last access. The caller
     *        should re-read the device descriptor and restart the operation.
     */
    MediaChanged = 9,

    /**
     * @brief A removable device has no media inserted.
     */
    NoMedia = 10
  };

  /**
   * @brief Classifies the physical medium attached to a block device.
   */
  enum class StorageDeviceType : UInt32 {
    /**
     * @brief Unknown or unclassified device type.
     */
    Unknown = 0,

    /**
     * @brief Floppy disk drive (FDC 82077AA / NEC 765-compatible). Uses CHS
     *        geometry internally; the server exposes LBA addressing to callers.
     */
    Floppy = 1,

    /**
     * @brief ATA/IDE hard disk (PIO mode). Addressed by 28-bit LBA.
     */
    HardDisk = 2,

    /**
     * @brief ATAPI optical drive (CD-ROM, DVD). Read-only; 2048-byte sectors.
     */
    OpticalDisk = 3,

    /**
     * @brief Memory-backed virtual disk. Sector size and count are
     *        configurable.
     */
    RAMDisk = 4
  };

  /**
   * @brief Describes a registered block device.
   *
   * Returned inline in `GetDevicesResult` and `GetDeviceInfoResult`. All
   * string fields are null-terminated and space-padded to their array bounds.
   */
  struct StorageDeviceDescriptor {
    /**
     * @brief Opaque numeric identifier assigned by the storage server.
     *        Pass this value in operation requests to address this device.
     */
    UInt32 DeviceID;

    /**
     * @brief Physical medium classification.
     */
    StorageDeviceType Type;

    /**
     * @brief Short programmatic name (e.g. `"FDC0"`, `"ATA0P0"`).
     */
    char Name[32];

    /**
     * @brief Human-readable display name (e.g. `"Floppy Disk A"`,
     *        `"Primary Master"`).
     */
    char DisplayName[48];

    /**
     * @brief Total number of addressable sectors (LBA 0 through
     *        `SectorCount - 1`).
     */
    UInt64 SectorCount;

    /**
     * @brief Size of one sector in bytes (typically 512 for floppy and HDD,
     *        2048 for optical).
     */
    UInt32 SectorSize;

    /**
     * @brief `true` if the device does not support write operations (e.g.
     *        a CD-ROM or write-protected floppy).
     */
    bool ReadOnly;

    /**
     * @brief `true` if the medium can be removed while the system is running
     *        (floppy, optical). When `true`, callers should use
     *        `GetMediaStatus` to confirm media presence before I/O.
     */
    bool Removable;
  };

  /**
   * @brief Request for `GetDevices`. Returns descriptors for all registered
   *        devices inline in `GetDevicesResult`.
   */
  struct GetDevicesRequest
    : public ABIRequestWithReplyPort<StorageOperation>
  {
  };

  /**
   * @brief Request for `GetDeviceInfo`. Returns the descriptor for a single
   *        device.
   */
  struct GetDeviceInfoRequest
    : public ABIRequestWithReplyPort<StorageOperation>
  {
    /**
     * @brief ID of the device to query.
     */
    UInt32 DeviceID;
  };

  /**
   * @brief Request for `Read`. The server reads `SectorCount` sectors
   *        starting at `LBA` into the shared buffer identified by `BufferID`.
   *        The caller must have already created the buffer with
   *        `Memory::CreateShared(SectorCount * SectorSize)` and attached it to
   *        obtain a local pointer before sending this request.
   */
  struct ReadRequest : public ABIRequestWithReplyPort<StorageOperation> {
    /**
     * @brief ID of the device to read from.
     */
    UInt32 DeviceID;

    /**
     * @brief Logical block address of the first sector to read.
     */
    UInt64 LBA;

    /**
     * @brief Number of consecutive sectors to read.
     */
    UInt32 SectorCount;

    /**
     * @brief Shared buffer ID that the server will attach, fill with sector
     *        data, and detach before sending the reply.
     */
    Kernel::Memory::SharedBufferID BufferID;
  };

  /**
   * @brief Request for `Write`. The caller must have already created and
   *        attached the buffer and written the data to it before sending this
   *        request. The server attaches the buffer, reads `SectorCount *
   *        SectorSize` bytes from it, writes them to the device, and detaches.
   */
  struct WriteRequest : public ABIRequestWithReplyPort<StorageOperation> {
    /**
     * @brief ID of the device to write to.
     */
    UInt32 DeviceID;

    /**
     * @brief Logical block address of the first sector to write.
     */
    UInt64 LBA;

    /**
     * @brief Number of consecutive sectors to write.
     */
    UInt32 SectorCount;

    /**
     * @brief Shared buffer ID containing the data to write.
     */
    Kernel::Memory::SharedBufferID BufferID;
  };

  /**
   * @brief Request for `Flush`.
   */
  struct FlushRequest : public ABIRequestWithReplyPort<StorageOperation> {
    /**
     * @brief ID of the device whose cache to flush.
     */
    UInt32 DeviceID;
  };

  /**
   * @brief Request for `GetMediaStatus`.
   */
  struct GetMediaStatusRequest
    : public ABIRequestWithReplyPort<StorageOperation>
  {
    /**
     * @brief ID of the device to poll.
     */
    UInt32 DeviceID;
  };

  /**
   * @brief Maximum number of devices a single driver process may register in
   *        one `RegisterDriverRequest`. A driver managing more than four
   *        devices must send multiple registrations.
   */
  constexpr UInt32 MaxDevicesPerDriver = 4;

  /**
   * @brief Registration message sent by a driver process to the Storage
   *        server once hardware probing is complete. Fire-and-forget: the
   *        driver does not wait for a reply.
   *
   * The `DriverPortID` field carries the auto-assigned IPC port on which the
   * Storage server will send `DriverABI::ReadRequest`, `WriteRequest`,
   * `FlushRequest`, and `GetMediaStatusRequest` messages.
   */
  struct RegisterDriverRequest : public ABIRequest<StorageOperation> {
    /**
     * @brief IPC port ID the driver process opened with Manage+Receive rights.
     *        The Storage server opens this port with Send rights to forward
     *        I/O requests.
     */
    Kernel::IPC::IPCPortID DriverPortID;

    /**
     * @brief Number of valid entries in `Devices` (0–`MaxDevicesPerDriver`).
     */
    UInt32 DeviceCount;

    /**
     * @brief Descriptors for each device this driver manages. The zero-based
     *        array index becomes the `LocalDeviceIndex` embedded in subsequent
     *        `DriverABI::ReadRequest` / `WriteRequest` messages, so the driver
     *        must maintain a stable mapping from index to hardware device for
     *        the lifetime of the process.
     *
     * The `DeviceID` field of each descriptor is ignored on receipt; the
     * Storage server assigns the final numeric ID.
     */
    StorageDeviceDescriptor Devices[MaxDevicesPerDriver];
  };

  /**
   * @brief Reply payload for `GetDevices`.
   */
  struct GetDevicesResult {
    /**
     * @brief Number of valid entries in `Devices`.
     */
    UInt32 Count;

    /**
     * @brief Descriptors for all registered devices. Only the first `Count`
     *        entries are valid.
     */
    StorageDeviceDescriptor Devices[MaxDevices];
  };

  /**
   * @brief Reply payload for `GetDeviceInfo`.
   */
  struct GetDeviceInfoResult {
    /**
     * @brief `true` if a device with the requested ID was found.
     */
    bool Success;

    /**
     * @brief Error code, or `Error::None` on success.
     */
    StorageOperationErrorCode StorageOperationErrorCode;

    /**
     * @brief Descriptor for the requested device. Valid only when `Success`
     *        is `true`.
     */
    StorageDeviceDescriptor Descriptor;
  };

  /**
   * @brief Reply payload for `Read`.
   */
  struct ReadResult {
    /**
     * @brief `true` if the read completed without error.
     */
    bool Success;

    /**
     * @brief Error code, or `Error::None` on success.
     */
    StorageOperationErrorCode StorageOperationErrorCode;

    /**
     * @brief Number of bytes actually read into the shared buffer. May be
     *        less than `SectorCount * SectorSize` on a partial error.
     */
    UInt32 BytesRead;
  };

  /**
   * @brief Reply payload for `Write`.
   */
  struct WriteResult {
    /**
     * @brief `true` if the write completed without error.
     */
    bool Success;

    /**
     * @brief Error code, or `Error::None` on success.
     */
    StorageOperationErrorCode StorageOperationErrorCode;

    /**
     * @brief Number of bytes actually written from the shared buffer.
     */
    UInt32 BytesWritten;
  };

  /**
   * @brief Reply payload for `Flush` and `GetMediaStatus`.
   */
  struct StatusResult {
    /**
     * @brief `true` if the operation succeeded (or, for `GetMediaStatus`,
     *        if media is present and unchanged).
     */
    bool Success;

    /**
     * @brief Error code, or `Error::None` on success.
     */
    StorageOperationErrorCode StorageOperationErrorCode;
  };

  /**
   * @brief Returns descriptors for all storage devices currently registered
   *        with the server.
   * @param outResult Pointer to receive the result.
   * @return `true` on success; `false` if the IPC call failed.
   */
  inline bool GetDevices(GetDevicesResult* outResult) {
    using namespace Kernel;

    Clients::KernelClient kernelClient;
    kernelClient.WaitForIPCPort(PortID);

    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(
      PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) {
      return false;
    }

    IPCPortID replyPortID = static_cast<IPCPortID>(
      500 + Kernel::ABI::Process::GetID() * 6
    );

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    GetDevicesRequest request;

    request.ABIVersion   = Version;
    request.Operation    = StorageOperation::GetDevices;
    request.ReplyPortID  = replyPortID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(GetDevicesResult)) {
      if (outResult) {
        *outResult = *static_cast<const GetDevicesResult*>(reply->Payload);
      }

      success = true;
    }

    if (reply) free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Returns the descriptor for a single storage device.
   * @param deviceID The ID of the device to query.
   * @param outResult Pointer to receive the result.
   * @return `true` on success; `false` if the IPC call failed.
   */
  inline bool GetDeviceInfo(UInt32 deviceID, GetDeviceInfoResult* outResult) {
    using namespace Kernel;

    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(
      PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      501 + Kernel::ABI::Process::GetID() * 6
    );
    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    GetDeviceInfoRequest request;

    request.ABIVersion  = Version;
    request.Operation   = StorageOperation::GetDeviceInfo;
    request.ReplyPortID = replyPortID;
    request.DeviceID    = deviceID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(GetDeviceInfoResult)) {
      if (outResult) {
        *outResult = *static_cast<const GetDeviceInfoResult*>(reply->Payload);
      }

      success = true;
    }

    if (reply) free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Reads sectors from a block device into a caller-managed shared
   *        memory buffer.
   *
   * The caller is responsible for the shared buffer lifecycle:
   * @code
   *   SharedBufferID id  = Memory::CreateShared(count * sectorSize);
   *   UIntPtr ptr = Memory::ABI::AttachShared(id);
   *   Storage::ABI::ReadResult r;
   *   Storage::ABI::Read(deviceID, lba, count, id, &r);
   *
   *   // read data from ptr
   *   Memory::ABI::DetachShared(ptr);
   * @endcode
   *
   * @param deviceID ID of the device to read from.
   * @param lba First sector to read (logical block address).
   * @param sectorCount Number of sectors to read.
   * @param bufferID Shared buffer ID (must be large enough for
   *                 `sectorCount * SectorSize` bytes).
   * @param outResult Pointer to receive the result.
   * @return `true` on success; `false` if the IPC call failed.
   */
  inline bool Read(
    UInt32 deviceID,
    UInt64 lba,
    UInt32 sectorCount,
    Kernel::Memory::SharedBufferID bufferID,
    ReadResult* outResult
  ) {
    using namespace Kernel;

    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(
      PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      502 + Kernel::ABI::Process::GetID() * 6
    );

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    ReadRequest request;

    request.ABIVersion = Version;
    request.Operation = StorageOperation::Read;
    request.ReplyPortID = replyPortID;
    request.DeviceID = deviceID;
    request.LBA = lba;
    request.SectorCount = sectorCount;
    request.BufferID = bufferID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(ReadResult)) {
      if (outResult) {
        *outResult = *static_cast<const ReadResult*>(reply->Payload);
      }

      success = true;
    }

    if (reply) free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Writes sectors to a block device from a caller-managed shared
   *        memory buffer.
   *
   * The caller must have filled the buffer with the data to write before
   * sending this request. See `Read` for the shared buffer lifecycle pattern.
   *
   * @param deviceID    ID of the device to write to.
   * @param lba         First sector to write (logical block address).
   * @param sectorCount Number of sectors to write.
   * @param bufferID    Shared buffer ID containing `sectorCount * SectorSize`
   *                    bytes of data to write.
   * @param outResult   Pointer to receive the result.
   * @return `true` on success; `false` if the IPC call failed.
   */
  inline bool Write(
    UInt32 deviceID,
    UInt64 lba,
    UInt32 sectorCount,
    Kernel::Memory::SharedBufferID bufferID,
    WriteResult* outResult
  ) {
    using namespace Kernel;

    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(
      PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      503 + Kernel::ABI::Process::GetID() * 6
    );
    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    WriteRequest request;

    request.ABIVersion = Version;
    request.Operation = StorageOperation::Write;
    request.ReplyPortID = replyPortID;
    request.DeviceID = deviceID;
    request.LBA = lba;
    request.SectorCount = sectorCount;
    request.BufferID = bufferID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(WriteResult)) {
      if (outResult) {
        *outResult = *static_cast<const WriteResult*>(reply->Payload);
      }

      success = true;
    }

    if (reply) free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Flushes any write-back cache for the specified device.
   * @param deviceID  ID of the device to flush.
   * @param outResult Pointer to receive the result.
   * @return `true` on success; `false` if the IPC call failed.
   */
  inline bool Flush(UInt32 deviceID, StatusResult* outResult) {
    using namespace Kernel;

    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(
      PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      504 + Kernel::ABI::Process::GetID() * 6
    );
    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    FlushRequest request;

    request.ABIVersion = Version;
    request.Operation = StorageOperation::Flush;
    request.ReplyPortID = replyPortID;
    request.DeviceID = deviceID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(StatusResult)) {
      if (outResult) {
        *outResult = *static_cast<const StatusResult*>(reply->Payload);
      }

      success = true;
    }

    if (reply) free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Queries whether a removable device currently has media inserted
   *        and that the media has not changed since the last access.
   * @param deviceID  ID of the device to poll.
   * @param outResult Pointer to receive the result.
   * @return `true` on success; `false` if the IPC call failed.
   */
  inline bool GetMediaStatus(UInt32 deviceID, StatusResult* outResult) {
    using namespace Kernel;

    IPCPortResourceID sendHandle = Kernel::ABI::IPC::Open(
      PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return false;

    IPCPortID replyPortID = static_cast<IPCPortID>(
      505 + Kernel::ABI::Process::GetID() * 6
    );

    IPCPortResourceID replyHandle = Kernel::ABI::IPC::Open(
      replyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
      Kernel::ABI::IPC::Close(sendHandle);

      return false;
    }

    GetMediaStatusRequest request;

    request.ABIVersion = Version;
    request.Operation = StorageOperation::GetMediaStatus;
    request.ReplyPortID = replyPortID;
    request.DeviceID = deviceID;

    Kernel::ABI::IPC::Send(sendHandle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(sendHandle);

    IPCMessage* reply = Kernel::ABI::IPC::Receive(replyHandle);

    bool success = false;

    if (reply && reply->PayloadSizeInBytes >= sizeof(StatusResult)) {
      if (outResult) {
        *outResult = *static_cast<const StatusResult*>(reply->Payload);
      }

      success = true;
    }

    if (reply) free(reply);

    Kernel::ABI::IPC::Close(replyHandle);

    return success;
  }

  /**
   * @brief Registers a storage driver with the Storage server. Spins until
   *        the Storage server's port is available, then sends a
   *        fire-and-forget `RegisterDriverRequest`.
   * @param driverPortID The IPC port ID that the driver opened for I/O
   *        requests from the Storage server.
   * @param deviceCount Number of devices to register
   *        (0–`MaxDevicesPerDriver`).
   * @param devices Array of device descriptors.
   */
  inline void RegisterDriver(
    Kernel::IPC::IPCPortID driverPortID,
    UInt32 deviceCount,
    const StorageDeviceDescriptor* devices
  ) {
    RegisterDriverRequest request;

    request.ABIVersion = Version;
    request.Operation = StorageOperation::RegisterDriver;
    request.DriverPortID = driverPortID;
    request.DeviceCount = deviceCount;

    for (UInt32 i = 0; i < deviceCount && i < MaxDevicesPerDriver; i++) {
      request.Devices[i] = devices[i];
    }

    Kernel::IPC::IPCPortResourceID handle;

    for (;;) {
      handle = Kernel::ABI::IPC::Open(PortID, Kernel::IPC::IPCPortRights::Send);

      if (handle != static_cast<Kernel::IPC::IPCPortResourceID>(-1)) break;

      Kernel::ABI::Thread::Yield();
    }

    Kernel::ABI::IPC::Send(handle, &request, sizeof(request));
    Kernel::ABI::IPC::Close(handle);
  }
}
