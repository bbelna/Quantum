/**
 * @file Servers/Core/Server.cpp
 * @brief Implements @ref @QSrvCore::Server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include <ServerCoreTypes.hpp>

#include "RequestController.hpp"
#include "Server.hpp"

namespace Quantum::Servers::Core {
  Server::Server(IPCPortID portID) :
    _kernel(),
    _portID(portID),
    _portHandle(
      _kernel.OpenIPCPort(
        portID,
        IPCPortRights::Manage | IPCPortRights::Receive,
        &_portID
      )
    ),
    _controllerEntries{},
    _controllerEntryCount(0)
  {
  }

  Server::~Server() {
    if (_portHandle != static_cast<IPCPortResourceID>(-1)) {
      _kernel.CloseIPCPort(_portHandle);
    }
  }

  Int32 Server::Run() {
    if (_portHandle == static_cast<IPCPortResourceID>(-1)) return -1;

    for (;;) {
      IPCMessage* message = _kernel.ReceiveIPCMessage(_portHandle);

      if (!message) continue;

      Int32 result = ProcessNextMessage(message);

      free(message);

      if (result != 0) return result;
    }
  }

  void Server::RegisterController(
    UInt32 operation,
    RequestController& controller
  ) {
    // check for existing entry with this operation (last-wins)
    for (Size i = 0; i < _controllerEntryCount; i++) {
      if (_controllerEntries[i].Operation == operation) {
        _controllerEntries[i].Controller = &controller;

        return;
      }
    }

    if (_controllerEntryCount >= MaxControllerEntries) return;

    _controllerEntries[_controllerEntryCount].Operation = operation;
    _controllerEntries[_controllerEntryCount].Controller = &controller;
    _controllerEntryCount++;
  }

  Int32 Server::ProcessNextMessage(IPCMessage* message) {
    if (!message->Payload) return 0;

    if (message->PayloadSizeInBytes < sizeof(ABIRequest<UInt32>)) return 0;

    auto* request = reinterpret_cast<const ABIRequest<UInt32>*>(
      message->Payload
    );

    UInt32 operation = request->Operation;

    for (Size i = 0; i < _controllerEntryCount; i++) {
      if (_controllerEntries[i].Operation == operation) {
        _controllerEntries[i].Controller->Handle(operation, message);

        return 0;
      }
    }

    return 0;
  }
}
