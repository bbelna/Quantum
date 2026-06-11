/**
 * @file Clients/InputClient.cpp
 * @brief Implements @ref @QClients::InputClient.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ClientsTypes.hpp>
#include "InputClient.hpp"

namespace Quantum::Clients {
  InputClient::~InputClient() {
    if (!_initialized) return;

    if (_replyHandle != static_cast<IPCPortResourceID>(-1)) {
      _kernel.CloseIPCPort(_replyHandle);
    }

    if (_sendHandle != static_cast<IPCPortResourceID>(-1)) {
      _kernel.CloseIPCPort(_sendHandle);
    }
  }

  bool InputClient::Initialize() {
    if (_initialized) return true;

    _kernel.WaitForIPCPort(InputPortID);

    _sendHandle = _kernel.OpenIPCPort(
      InputPortID,
      IPCPortRights::Send
    );

    if (_sendHandle == static_cast<IPCPortResourceID>(-1)) {
      return false;
    }

    _replyPortID = static_cast<IPCPortID>(-1);

    _replyHandle = _kernel.OpenIPCPort(
      static_cast<IPCPortID>(-1),
      IPCPortRights::Manage | IPCPortRights::Receive,
      &_replyPortID
    );

    if (_replyHandle == static_cast<IPCPortResourceID>(-1)) {
      _kernel.CloseIPCPort(_sendHandle);
      _sendHandle = static_cast<IPCPortResourceID>(-1);

      return false;
    }

    _initialized = true;

    return true;
  }

  bool InputClient::GetNextEvent(Input::InputEvent* outEvent) {
    if (!_initialized) return false;

    InputGetNextEventRequest request = {};

    request.ABIVersion = InputABIVersion;
    request.Operation = InputOperation::GetNextEvent;
    request.ReplyPortID = _replyPortID;

    _kernel.SendIPCMessage(
      _sendHandle,
      &request,
      sizeof(InputGetNextEventRequest)
    );

    IPCMessage* reply = _kernel.ReceiveIPCMessage(_replyHandle);

    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(InputGetNextEventResult)
    ) {
      auto* result = static_cast<const InputGetNextEventResult*>(
        reply->Payload
      );

      if (result->HasEvent && outEvent) {
        *outEvent = result->Event;
        success = true;
      }
    }

    if (reply) {
      free(reply);
    }

    return success;
  }

  bool InputClient::TryGetNextEvent(Input::InputEvent* outEvent) {
    if (!_initialized) return false;

    InputGetNextEventRequest request = {};

    request.ABIVersion = InputABIVersion;
    request.Operation = InputOperation::TryGetNextEvent;
    request.ReplyPortID = _replyPortID;

    _kernel.SendIPCMessage(
      _sendHandle,
      &request,
      sizeof(InputGetNextEventRequest)
    );

    IPCMessage* reply = _kernel.ReceiveIPCMessage(_replyHandle);

    bool success = false;

    if (
      reply &&
      reply->PayloadSizeInBytes >= sizeof(InputGetNextEventResult)
    ) {
      auto* result = static_cast<const InputGetNextEventResult*>(
        reply->Payload
      );

      if (result->HasEvent && outEvent) {
        *outEvent = result->Event;
        success = true;
      }
    }

    if (reply) {
      free(reply);
    }

    return success;
  }
}
