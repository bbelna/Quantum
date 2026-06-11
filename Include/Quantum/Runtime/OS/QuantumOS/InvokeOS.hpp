/**
 * @file Include/Quantum/Runtime/QuantumInvokeOS.hpp
 * @brief Implements @ref InvokeOS for IPC-based server invocations.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Kernel.hpp>

#include "../../InvokeOS.hpp"

/**
 * @brief QuantumOS implementation of @ref InvokeOS.
 *
 * @p PayloadType is expected to inherit from @ref ABIRequestWithReplyPort so
 * that the @c ReplyPortID field can be set automatically before the message
 * is sent.
 *
 * @ref InvokeOS performs the following steps, in order:
 *
 *   1. Opens a send handle to the IPC port identified by @p subsystem.
 * 
 *   2. Creates a temporary auto-assigned reply port with Manage and Receive
 *      rights.
 *
 *   3. Sets @c payload.ReplyPortID to the auto-assigned reply port ID.
 *
 *   4. Sends the @p payload as the IPC message body.
 *
 *   5. Blocks on the reply port until a response arrives.
 *
 *   6. Copies @c sizeof(ResultType) bytes from the reply payload into the
 *      return value.
 *
 *   7. Frees the reply message and closes both IPC handles.
 */
template <typename ResultType, typename PayloadType>
ResultType InvokeOS(
  UInt32 subsystem,
  UInt32 operation,
  PayloadType payload
) {
  namespace KIPC = Quantum::Kernel::ABI::IPC;
  namespace KMem = Quantum::Kernel::Memory::ABI;

  using Quantum::Kernel::IPC::IPCPortID;
  using Quantum::Kernel::IPC::IPCPortResourceID;
  using Quantum::Kernel::IPC::IPCPortRights;
  using Quantum::Kernel::IPC::IPCMessage;

  // open a send handle to the server's IPC port
  IPCPortResourceID sendHandle = KIPC::Open(
    static_cast<IPCPortID>(subsystem),
    IPCPortRights::Send
  );

  if (sendHandle == static_cast<IPCPortResourceID>(-1))
    return ResultType{};

  // create a temporary reply port
  IPCPortID replyPortID;
  IPCPortResourceID replyHandle = KIPC::Open(
    static_cast<IPCPortID>(-1),
    IPCPortRights::Manage | IPCPortRights::Receive,
    &replyPortID
  );

  if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
    KIPC::Close(sendHandle);

    return ResultType{};
  }

  // stamp the reply port into the payload and send
  payload.ReplyPortID = replyPortID;

  KIPC::Send(
    sendHandle,
    static_cast<const void*>(&payload),
    sizeof(PayloadType)
  );

  KIPC::Close(sendHandle);

  // wait for the reply
  IPCMessage* reply = KIPC::Receive(replyHandle);

  ResultType result{};

  if (reply && reply->PayloadSizeInBytes >= sizeof(ResultType)) {
    Quantum::Core::Byte::Copy(
      &result,
      reply->Payload,
      sizeof(ResultType)
    );
  }

  if (reply) {
    free(reply);
  }

  KIPC::Close(replyHandle);

  return result;
}

/**
 * @brief QuantumOS implementation of @ref InvokeOSRaw.
 *
 * Performs the same IPC handshake as @ref InvokeOS but returns the raw
 * reply message instead of copying a fixed-size result. The caller must
 * free the returned message via `free()` or `delete`.
 */
template <typename PayloadType>
Quantum::Kernel::IPC::IPCMessage* InvokeOSRaw(
  UInt32 subsystem,
  PayloadType payload
) {
  namespace KIPC = Quantum::Kernel::ABI::IPC;

  using Quantum::Kernel::IPC::IPCPortID;
  using Quantum::Kernel::IPC::IPCPortResourceID;
  using Quantum::Kernel::IPC::IPCPortRights;

  IPCPortResourceID sendHandle = KIPC::Open(
    static_cast<IPCPortID>(subsystem),
    IPCPortRights::Send
  );

  if (sendHandle == static_cast<IPCPortResourceID>(-1)) {
    return nullptr;
  }

  IPCPortID replyPortID;
  IPCPortResourceID replyHandle = KIPC::Open(
    static_cast<IPCPortID>(-1),
    IPCPortRights::Manage | IPCPortRights::Receive,
    &replyPortID
  );

  if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
    KIPC::Close(sendHandle);

    return nullptr;
  }

  payload.ReplyPortID = replyPortID;

  KIPC::Send(
    sendHandle,
    static_cast<const void*>(&payload),
    sizeof(PayloadType)
  );

  KIPC::Close(sendHandle);

  Quantum::Kernel::IPC::IPCMessage* reply = KIPC::Receive(replyHandle);

  KIPC::Close(replyHandle);

  return reply;
}

/**
 * @brief QuantumOS implementation of @ref InvokeOSBuffer.
 *
 * Sends a caller-managed, variable-size payload buffer to a subsystem
 * and returns a fixed-size result extracted from the reply.
 */
template <typename ResultType, typename HeaderType>
ResultType InvokeOSBuffer(
  UInt32 subsystem,
  HeaderType* payload,
  Size payloadSize
) {
  namespace KIPC = Quantum::Kernel::ABI::IPC;

  using Quantum::Kernel::IPC::IPCPortID;
  using Quantum::Kernel::IPC::IPCPortResourceID;
  using Quantum::Kernel::IPC::IPCPortRights;
  using Quantum::Kernel::IPC::IPCMessage;

  if (!payload) return ResultType{};

  IPCPortResourceID sendHandle = KIPC::Open(
    static_cast<IPCPortID>(subsystem),
    IPCPortRights::Send
  );

  if (sendHandle == static_cast<IPCPortResourceID>(-1)) {
    return ResultType{};
  }

  IPCPortID replyPortID;
  IPCPortResourceID replyHandle = KIPC::Open(
    static_cast<IPCPortID>(-1),
    IPCPortRights::Manage | IPCPortRights::Receive,
    &replyPortID
  );

  if (replyHandle == static_cast<IPCPortResourceID>(-1)) {
    KIPC::Close(sendHandle);

    return ResultType{};
  }

  payload->ReplyPortID = replyPortID;

  KIPC::Send(
    sendHandle,
    static_cast<const void*>(payload),
    payloadSize
  );

  KIPC::Close(sendHandle);

  IPCMessage* reply = KIPC::Receive(replyHandle);

  ResultType result{};

  if (reply && reply->PayloadSizeInBytes >= sizeof(ResultType)) {
    Quantum::Core::Byte::Copy(
      &result,
      reply->Payload,
      sizeof(ResultType)
    );
  }

  if (reply) {
    free(reply);
  }

  KIPC::Close(replyHandle);

  return result;
}

/**
 * @brief Forward declaration of the kernel syscall specialization of
 *        @ref InvokeOS.
 *
 * The full definition lives in `Runtime/QuantumInvokeOS.cpp`. This
 * declaration prevents the compiler from instantiating the generic
 * IPC-based template when @ref KernelOperationPayload is used.
 */
template <>
UInt32 InvokeOS<UInt32, Quantum::Kernel::KernelOperationPayload>(
  UInt32 subsystem,
  UInt32 operation,
  Quantum::Kernel::KernelOperationPayload payload
);

/**
 * @brief Forward declaration of @ref InvokeOSEx.
 *
 * The full definition lives in `Runtime/QuantumInvokeOS.cpp`.
 */
Quantum::Kernel::KernelOperationResult InvokeOSEx(
  UInt32 operation,
  Quantum::Kernel::KernelOperationPayload payload
);
