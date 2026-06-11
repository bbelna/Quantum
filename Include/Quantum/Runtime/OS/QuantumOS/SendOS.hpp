/**
 * @file Include/Quantum/Runtime/QuantumSendOS.hpp
 * @brief Implements @ref SendOS for fire-and-forget IPC messages.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core.hpp>
#include <Quantum/Kernel.hpp>

#include "../../SendOS.hpp"

/**
 * @brief QuantumOS implementation of @ref SendOS.
 *
 * Opens a send handle to the IPC port identified by @p subsystem, sends the
 * @p payload, and closes the handle. No reply port is created and no response
 * is awaited.
 */
template <typename PayloadType>
void SendOS(UInt32 subsystem, PayloadType payload) {
  namespace KIPC = Quantum::Kernel::ABI::IPC;

  using Quantum::Kernel::IPC::IPCPortID;
  using Quantum::Kernel::IPC::IPCPortResourceID;
  using Quantum::Kernel::IPC::IPCPortRights;

  IPCPortResourceID sendHandle = KIPC::Open(
    static_cast<IPCPortID>(subsystem),
    IPCPortRights::Send
  );

  if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

  KIPC::Send(
    sendHandle,
    static_cast<const void*>(&payload),
    sizeof(PayloadType)
  );

  KIPC::Close(sendHandle);
}
