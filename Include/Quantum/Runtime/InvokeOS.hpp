/**
 * @file Include/Runtime/InvokeOS.hpp
 * @brief Declares @ref InvokeOS.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Kernel {
  struct KernelOperationResult;
  struct KernelOperationPayload;
}

namespace Quantum::Kernel::IPC { struct IPCMessage; }

/**
 * @brief Invokes the specified Operating System (OS) subsystem, optionally
 *        with a payload, and returns a result.
 * @param subsystem Subsystem identifier.
 * @param operation Operation code.
 * @param payload Optional payload.
 * @return The result of the host OS invocation, if applicable.
 */
template <typename ResultType, typename PayloadType>
ResultType InvokeOS(
  UInt32 subsystem,
  UInt32 operation,
  PayloadType payload = PayloadType{}
);

/**
 * @brief Invokes the specified Operating System (OS) subsystem and returns
 *        the raw IPC reply message. Use this variant for operations that
 *        return variable-length data (flexible array members).
 *
 * The caller is responsible for freeing the returned message via
 * `free()` or `delete`.
 *
 * @param subsystem Subsystem identifier (IPC port ID).
 * @param payload The request payload to send.
 * @return Pointer to the reply message, or `nullptr` on failure.
 */
template <typename PayloadType>
Quantum::Kernel::IPC::IPCMessage* InvokeOSRaw(
  UInt32 subsystem,
  PayloadType payload
);

/**
 * @brief Invokes an OS subsystem with a variable-size payload buffer and
 *        returns a fixed-size result.
 *
 * Like @ref InvokeOS, but the send payload is a caller-managed buffer with
 * a runtime size, allowing trailing variable-length data (e.g. inline file
 * contents). The buffer's header must contain a `ReplyPortID` field at the
 * offset defined by @p HeaderType.
 *
 * @tparam ResultType The fixed-size result type to extract from the reply.
 * @tparam HeaderType The request header type (must have a `ReplyPortID`
 *                    field).
 * @param subsystem Subsystem identifier (IPC port ID).
 * @param payload Pointer to the request buffer.
 * @param payloadSize Total size of the buffer in bytes.
 * @return The extracted result, or a zero-initialized value on failure.
 */
template <typename ResultType, typename HeaderType>
ResultType InvokeOSBuffer(
  UInt32 subsystem,
  HeaderType* payload,
  Size payloadSize
);

/**
 * @brief Invokes a kernel syscall and returns all register outputs.
 *
 * Like @ref InvokeOS, but returns a @ref KernelOperationResult containing the
 * primary result and all three output arguments as written back by the
 * kernel. Use this for operations that return multiple values.
 *
 * @param operation Operation code.
 * @param payload Payload arguments.
 * @return A @ref KernelOperationResult with all outputs.
 */
Quantum::Kernel::KernelOperationResult InvokeOSEx(
  UInt32 operation,
  Quantum::Kernel::KernelOperationPayload payload
);
