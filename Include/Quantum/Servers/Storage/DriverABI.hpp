/**
 * @file Include/Quantum/Servers/Storage/DriverABI.hpp
 * @brief Internal IPC protocol between the Storage server and hardware driver
 *        processes (Floppy, ATA, etc.).
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI/ABIRequest.hpp>
#include <Quantum/Kernel.hpp>
#include <Quantum/Servers/Storage/ABI.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::Servers::Storage::DriverABI {
  /**
   * @brief IPC port the Storage server opens with Manage+Receive rights to
   *        receive replies from driver processes. Drivers send their results
   *        to this port ID with Send rights.
   */
  constexpr Kernel::IPC::IPCPortID ReplyPortID = 650;

  /**
   * @brief Operations that the Storage server sends to a driver process.
   */
  enum class Operation : UInt32 {
    /**
     * @brief Read one or more sectors from the device into a caller-managed
     *        shared buffer. The driver attaches `BufferID`, performs the
     *        hardware read, and detaches before sending the reply.
     */
    Read = 1,

    /**
     * @brief Write one or more sectors to the device from a caller-managed
     *        shared buffer. The driver attaches `BufferID`, reads the data,
     *        performs the hardware write, and detaches before sending the
     *        reply.
     */
    Write = 2,

    /**
     * @brief Flush any internal write-back cache to the physical medium. For
     *        devices without a cache (e.g., floppy) this is a no-op.
     */
    Flush = 3,

    /**
     * @brief Query the media-presence state for removable devices.
     */
    GetMediaStatus = 4
  };

  /**
   * @brief Base request type for all Storage -> Driver operations.
   */
  using Request = ABIRequest<Operation>;

  // ---------------------------------------------------------------------------
  // Request structures (Storage -> Driver)
  // ---------------------------------------------------------------------------

  /**
   * @brief Request for a sector read operation.
   */
  struct ReadRequest : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief Zero-based index identifying the specific device within this
     *        driver process. Corresponds to the position in the
     *        `RegisterDriverRequest::Devices` array sent during registration.
     */
    UInt32 LocalDeviceIndex;

    /**
     * @brief Logical block address of the first sector to read.
     */
    UInt64 LBA;

    /**
     * @brief Number of consecutive sectors to read.
     */
    UInt32 SectorCount;

    /**
     * @brief Shared buffer ID. The driver must call `Memory::AttachShared` to
     *        obtain a writable virtual address, fill it with sector data, and
     *        call `Memory::DetachShared` before sending the reply.
     */
    Kernel::Memory::SharedBufferID BufferID;
  };

  /**
   * @brief Request for a sector write operation.
   */
  struct WriteRequest : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief Zero-based device index within this driver process.
     */
    UInt32 LocalDeviceIndex;

    /**
     * @brief Logical block address of the first sector to write.
     */
    UInt64 LBA;

    /**
     * @brief Number of consecutive sectors to write.
     */
    UInt32 SectorCount;

    /**
     * @brief Shared buffer ID containing the data to write. The driver must
     *        attach, read the data, write it to hardware, and detach before
     *        sending the reply.
     */
    Kernel::Memory::SharedBufferID BufferID;
  };

  /**
   * @brief Request to flush any write-back cache.
   */
  struct FlushRequest : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief Zero-based device index within this driver process.
     */
    UInt32 LocalDeviceIndex;
  };

  /**
   * @brief Request to query media presence for removable devices.
   */
  struct GetMediaStatusRequest : public ABIRequestWithReplyPort<Operation> {
    /**
     * @brief Zero-based device index within this driver process.
     */
    UInt32 LocalDeviceIndex;
  };

  // ---------------------------------------------------------------------------
  // Result structures (Driver -> Storage)
  // ---------------------------------------------------------------------------

  /**
   * @brief Reply payload for a `Read` operation.
   */
  struct ReadResult {
    /**
     * @brief `true` if the read completed without error.
     */
    bool Success;

    /**
     * @brief Error code, or `ABI::Error::None` on success.
     */
    ABI::StorageOperationErrorCode StorageOperationErrorCode;

    /**
     * @brief Number of bytes actually read. Zero on failure.
     */
    UInt32 BytesRead;
  };

  /**
   * @brief Reply payload for a `Write` operation.
   */
  struct WriteResult {
    /**
     * @brief `true` if the write completed without error.
     */
    bool Success;

    /**
     * @brief Error code, or `ABI::Error::None` on success.
     */
    ABI::StorageOperationErrorCode StorageOperationErrorCode;

    /**
     * @brief Number of bytes actually written. Zero on failure.
     */
    UInt32 BytesWritten;
  };

  /**
   * @brief Reply payload for `Flush` and `GetMediaStatus` operations.
   */
  struct StatusResult {
    /**
     * @brief `true` if the operation succeeded.
     */
    bool Success;

    /**
     * @brief Error code, or `ABI::Error::None` on success.
     */
    ABI::StorageOperationErrorCode StorageOperationErrorCode;
  };
}
