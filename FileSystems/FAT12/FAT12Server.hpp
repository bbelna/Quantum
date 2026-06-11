/**
 * @file FileSystems/FAT12/FAT12Server.hpp
 * @brief Declares the FAT12 file system backend server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Kernel.hpp>
#include <Quantum/Servers/FileSystem/ABI.hpp>
#include <Quantum/Core/Types.hpp>

#include "Volume.hpp"
#include "HandleManager.hpp"

namespace Quantum::FileSystems::FAT12 {
  /**
   * @brief FAT12 file system backend service.
   *
   * The server opens an auto-assigned IPC port, locates the first floppy
   * storage device, mounts a FAT12 volume on it, registers the volume as
   * `"System"` with the file system server, and then enters a message loop
   * handling forwarded file system ABI operations.
   *
   * Supported operations: `Open`, `Close`, `Read`, `Stat`, `ReadDirectory`.
   * `Write`, `Delete`, and `CreateDirectory` return `InvalidOperation`
   * (read-only).
   */
  class FAT12Server {
    public:
      /**
       * @brief Starts the FAT12 server for the given storage device.
       *        Never returns on success.
       * @param storageDeviceID The storage device ID to mount.
       * @param deviceName Null-terminated device name (e.g., `"FDC0"`)
       *        used as the volume ID when registering with the file system
       *        server.
       */
      void Start(UInt32 storageDeviceID, const char* deviceName);

    private:
      /**
       * @brief Mounted FAT12 volume.
       */
      Volume _volume;

      /**
       * @brief Open file handle table.
       */
      HandleManager _handles;

      /**
       * @brief Handles a forwarded `Open` request.
       * @param message The raw IPC message received from the file system
       *                server.
       */
      void _handleOpen(const Kernel::IPC::IPCMessage* message);

      /**
       * @brief Handles a forwarded `Close` request.
       * @param message The raw IPC message received from the file system
       *                server.
       */
      void _handleClose(const Kernel::IPC::IPCMessage* message);

      /**
       * @brief Handles a forwarded `Read` request.
       * @param message The raw IPC message received from the file system
       *                server.
       */
      void _handleRead(const Kernel::IPC::IPCMessage* message);

      /**
       * @brief Handles a forwarded `Stat` request.
       * @param message The raw IPC message received from the file system
       *                server.
       */
      void _handleStat(const Kernel::IPC::IPCMessage* message);

      /**
       * @brief Handles a forwarded `ReadDirectory` request.
       * @param message The raw IPC message received from the file system
       *                server.
       */
      void _handleReadDirectory(const Kernel::IPC::IPCMessage* message);

      /**
       * @brief Opens a Send handle to `portID`, sends `payload`, then closes
       *        the handle.
       * @param portID Destination port ID.
       * @param payload Pointer to the message payload to send.
       * @param payloadSize Size of the message payload in bytes.
       */
      static void _sendReply(
        Kernel::IPC::IPCPortID portID,
        const void* payload,
        Size payloadSize
      );

      /**
       * @brief Sends a minimal error reply for unsupported operations.
       * @param replyPortID Destination port ID for the reply.
       */
      static void _sendInvalidOperation(Kernel::IPC::IPCPortID replyPortID);
  };
}
