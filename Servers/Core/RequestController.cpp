/**
 * @file Servers/Core/RequestController.cpp
 * @brief Implements @ref @QSrvCore::RequestController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ServerCoreTypes.hpp>

#include "RequestController.hpp"

namespace Quantum::Servers::Core {
  RequestController::RequestController(
    Clients::KernelClient& kernel,
    ServerLog& log
  ) :
    _kernel(kernel),
    _log(log)
  {
  }

  void RequestController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    for (Size i = 0; i < _typedHandlerCount; ++i) {
      if (_typedHandlers[i].Operation == operation) {
        if (message->PayloadSizeInBytes < _typedHandlers[i].MinPayloadSize) {
          return;
        }

        _typedHandlers[i].Invoke(this, message);

        return;
      }
    }
  }

  void RequestController::_addHandler(
    UInt32 operation,
    Size minPayloadSize,
    _HandlerInvokeFn invoke
  ) {
    if (_typedHandlerCount >= MaxTypedHandlers) return;

    _typedHandlers[_typedHandlerCount].Operation = operation;
    _typedHandlers[_typedHandlerCount].MinPayloadSize = minPayloadSize;
    _typedHandlers[_typedHandlerCount].Invoke = invoke;
    _typedHandlerCount++;
  }

  void RequestController::SendReply(
    IPCPortID replyPortID,
    const void* payload,
    Size payloadSize
  ) {
    IPCPortResourceID replyHandle = _kernel.OpenIPCPort(
      replyPortID, IPCPortRights::Send
    );

    if (replyHandle != static_cast<IPCPortResourceID>(-1)) {
      _kernel.SendIPCMessage(replyHandle, payload, payloadSize);
      _kernel.CloseIPCPort(replyHandle);
    }
  }
}
