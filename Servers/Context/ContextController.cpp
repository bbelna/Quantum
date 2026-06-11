/**
 * @file Servers/Context/ContextController.cpp
 * @brief Implements @ref @QCtxSrv::ContextController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ContextController.hpp"

namespace Quantum::Servers::Context {
  ContextController::ContextController(
    KernelClient& kernel,
    ServerLog& log,
    MenuService& menuService,
    MenuBar& menuBar
  ) :
    RequestController(kernel, log),
    _menuService(menuService),
    _menuBar(menuBar)
  {
  }

  void ContextController::Handle(
    UInt32 operation,
    const IPCMessage* message
  ) {
    switch (Enum::To<CtxABI::ContextOperation>(operation)) {
      case CtxABI::ContextOperation::RegisterProvider:
        _handleRegisterProvider(message);
        break;

      case CtxABI::ContextOperation::UnregisterProvider:
        _handleUnregisterProvider(message);
        break;

      case CtxABI::ContextOperation::SetContributions:
        _handleSetContributions(message);
        break;

      case CtxABI::ContextOperation::UpdateFocusContext:
        _handleUpdateFocusContext(message);
        break;

      case CtxABI::ContextOperation::InvokeAction:
        _handleInvokeAction(message);
        break;

      case CtxABI::ContextOperation::GetMenuBarState:
        _handleGetMenuBarState(message);
        break;

      default:
        break;
    }
  }

  void ContextController::_handleRegisterProvider(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(CtxABI::ContextRegisterProviderRequest)
    ) {
      _log.Warning("RegisterProvider request too small");

      return;
    }

    auto* request = static_cast<
      const CtxABI::ContextRegisterProviderRequest*
    >(message->Payload);

    ProviderID assignedID = _menuService.RegisterProvider(
      request->ProcessID,
      request->Layer,
      request->Priority
    );

    CtxABI::ContextRegisterProviderResult result = {};

    result.Success = (assignedID != 0);
    result.ProviderID = assignedID;

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void ContextController::_handleUnregisterProvider(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(CtxABI::ContextUnregisterProviderRequest)
    ) {
      return;
    }

    auto* request = static_cast<
      const CtxABI::ContextUnregisterProviderRequest*
    >(message->Payload);

    _menuService.UnregisterProvider(request->ProviderID);
  }

  void ContextController::_handleSetContributions(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(CtxABI::ContextSetContributionsRequest)
    ) {
      return;
    }

    auto* request = static_cast<
      const CtxABI::ContextSetContributionsRequest*
    >(message->Payload);

    _menuService.SetContributionsFromBuffer(
      request->ProviderID,
      request->BufferID
    );

    CtxABI::ContextSetContributionsAck ack = {};

    ack.Success = true;

    SendReply(request->ReplyPortID, &ack, sizeof(ack));

    NeedsRefresh = true;
  }

  void ContextController::_handleUpdateFocusContext(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(CtxABI::ContextUpdateFocusContextRequest)
    ) {
      return;
    }

    auto* request = static_cast<
      const CtxABI::ContextUpdateFocusContextRequest*
    >(message->Payload);

    _menuService.UpdateFocusContext(request->Context);

    if (_menuBar.IsDropdownOpen()) {
      _menuBar.CloseDropdown();
    }

    NeedsRefresh = true;
  }

  void ContextController::_handleInvokeAction(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(CtxABI::ContextInvokeActionRequest)
    ) {
      return;
    }

    auto* request = static_cast<
      const CtxABI::ContextInvokeActionRequest*
    >(message->Payload);

    CtxABI::ContextInvokeActionResult result = {};

    const ActionProviderMapping* mapping
      = _menuService.FindProviderForAction(request->Action);

    if (mapping) {
      AppServer::DeliverMenuAction(
        request->Context.WindowID,
        request->Action
      );

      result.Handled = true;
    } else {
      _log.Warning(
        "No provider found for action %u",
        request->Action
      );

      result.Handled = false;
    }

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }

  void ContextController::_handleGetMenuBarState(
    const IPCMessage* message
  ) {
    if (
      message->PayloadSizeInBytes
        < sizeof(CtxABI::ContextGetMenuBarStateRequest)
    ) {
      return;
    }

    auto* request = static_cast<
      const CtxABI::ContextGetMenuBarStateRequest*
    >(message->Payload);

    CtxABI::ContextGetMenuBarStateResult result = {};

    result.BufferID = _menuService.SerializeMenuBarState();
    result.Success = (result.BufferID != 0);

    SendReply(request->ReplyPortID, &result, sizeof(result));
  }
}
