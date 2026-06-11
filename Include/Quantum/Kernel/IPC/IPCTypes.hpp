/**
 * @file Include/Quantum/Kernel/Types/IPC.hpp
 * @brief Declaration of kernel-wide IPC types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>

#include "../Concurrency/ConcurrencyTypes.hpp"
#include "../Memory/MemoryTypes.hpp"

#define INVALID_IPC_PORT_ID \
  static_cast<Quantum::Kernel::IPC::IPCPortID>(-1)

#define INVALID_IPC_PORT_RESOURCE_ID \
  static_cast<Quantum::Kernel::IPC::IPCPortResourceID>(-1)

namespace Quantum::Kernel::IPC {
  /**
   * @brief Type for IPC port IDs.
   */
  using IPCPortID = UInt32;

  inline constexpr IPCPortID InvalidIPCPortID = INVALID_IPC_PORT_ID;

  /**
   * @brief Type for IPC port resource IDs.
   */
  using IPCPortResourceID = UInt32;

  inline constexpr IPCPortResourceID InvalidIPCPortResourceID
    = INVALID_IPC_PORT_RESOURCE_ID;

  /**
   * @brief Rights/permissions associated with an IPC port allocation.
   */
  enum class IPCPortRights : UInt32 {
    /**
     * @brief Right to receive messages from the IPC port.
     */
    Receive = 1u << 0,

    /**
     * @brief Right to send messages to the IPC port.
     */
    Send = 1u << 1,

    /**
     * @brief Right to manage the IPC port (e.g., change permissions, close
     *        the port, etc.).
     */
    Manage = 1u << 2
  };

  /**
   * @brief Distinguishes traditional copy-based IPC messages from
   *        zero-copy shared-buffer messages.
   */
  enum class IPCMessageType : UInt32 {
    /**
     * @brief Traditional payload-copy message (default).
     */
    Copy = 0,

    /**
     * @brief Zero-copy shared-buffer descriptor message.
     */
    Shared = 1
  };

  /**
   * @brief Describes a region within a shared buffer for zero-copy IPC.
   */
  struct IPCSharedDescriptor {
    /**
     * @brief ID of the shared buffer.
     */
    Memory::SharedBufferID BufferID = 0;

    /**
     * @brief Byte offset into the shared buffer where data begins.
     */
    UInt32 Offset = 0;

    /**
     * @brief Number of bytes of data starting at @ref Offset.
     */
    UInt32 Size = 0;
  };

  /**
   * @brief Structure representing an IPC message. Contains the ID of the
   *        sending process and any additional data fields as needed.
   */
  struct IPCMessage {
    /**
     * @brief ID of the process that sent this message.
     */
    Concurrency::ProcessID SendingProcessID;

    /**
     * @brief Optional payload data associated with this message. The actual
     *        structure and meaning of this data is defined by the sender and
     *        receiver processes.
     */
    void* Payload;

    /**
     * @brief Size of the payload data in bytes.
     */
    Size PayloadSizeInBytes;

    /**
     * @brief Message type: copy-based or shared-buffer.
     */
    IPCMessageType Type = IPCMessageType::Copy;

    /**
     * @brief Shared-buffer descriptor. Only valid when
     *        @ref Type is @ref IPCMessageType::Shared.
     */
    IPCSharedDescriptor SharedDescriptor;

    /**
     * @brief Intrusive queue link to the next message.
     */
    IPCMessage* Next = nullptr;

    /**
     * @brief Intrusive queue link to the previous message.
     */
    IPCMessage* Previous = nullptr;
  };
}

QUANTUM_ENABLE_ENUM_BITMASK_OPS(Quantum::Kernel::IPC, IPCPortRights)
