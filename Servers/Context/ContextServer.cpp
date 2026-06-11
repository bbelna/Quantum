/**
 * @file Servers/Context/ContextServer.cpp
 * @brief Implements @ref @QCtxSrv::ContextServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#include "ContextServer.hpp"

namespace Quantum::Servers::Context {
  /// Real-time scheduling priority for the context server (in the
  /// input->overlay->render path).
  static constexpr UInt32 ContextServerPriority = 27;

  ContextServer::ContextServer(
    StartupClient& startupClient,
    ServerLog& log,
    ContextController& contextController,
    MenuService& menuService,
    MenuBar& menuBar
  ) :
    Server(CtxABI::ContextPortID),
    _kernel(),
    _log(log),
    _contextController(contextController),
    _menuService(menuService),
    _menuBar(menuBar)
  {
    RegisterController(
      CtxABI::ContextOperation::RegisterProvider, contextController
    );
    RegisterController(
      CtxABI::ContextOperation::UnregisterProvider, contextController
    );
    RegisterController(
      CtxABI::ContextOperation::SetContributions, contextController
    );
    RegisterController(
      CtxABI::ContextOperation::UpdateFocusContext, contextController
    );
    RegisterController(
      CtxABI::ContextOperation::InvokeAction, contextController
    );
    RegisterController(
      CtxABI::ContextOperation::GetMenuBarState, contextController
    );

    startupClient.Ready();
  }

  Int32 ContextServer::Run() {
    if (_portHandle == static_cast<IPCPortResourceID>(-1)) {
      _log.Error("Failed to open IPC port");

      return -1;
    }

    _registerSystemMenus();

    _kernel.SetThreadPriority(ContextServerPriority);

    // event reply ports: one for menu bar, one for dropdown
    _eventReplyPortID = static_cast<IPCPortID>(
      600 + _kernel.GetProcessID() * 3
    );

    IPCPortID dropdownEventPortID = static_cast<IPCPortID>(
      600 + _kernel.GetProcessID() * 3 + 1
    );

    IPCPortResourceID menuBarEventHandle = _kernel.OpenIPCPort(
      _eventReplyPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    IPCPortResourceID dropdownEventHandle = _kernel.OpenIPCPort(
      dropdownEventPortID,
      IPCPortRights::Manage | IPCPortRights::Receive
    );

    bool hasEventPorts = (
      menuBarEventHandle != static_cast<IPCPortResourceID>(-1) &&
      dropdownEventHandle != static_cast<IPCPortResourceID>(-1)
    );

    _refreshMenuBar();

    for (;;) {
      IPCMessage* message = nullptr;

      if (hasEventPorts && _menuBar.IsValid()) {
        IPCPortResourceID handles[3] = {
          _portHandle, menuBarEventHandle, dropdownEventHandle
        };

        Size handleCount = _menuBar.IsDropdownOpen() ? 3 : 2;
        Size readyIndex = 0;

        message = _kernel.ReceiveAnyIPCMessage(
          handles, handleCount, &readyIndex
        );

        if (message && (readyIndex == 1 || readyIndex == 2)) {
          bool isDropdown = (readyIndex == 2);

          if (
            message->PayloadSizeInBytes
              >= sizeof(AppServer::WindowEventResult)
          ) {
            auto* eventResult = static_cast<
              const AppServer::WindowEventResult*
            >(message->Payload);

            if (eventResult->HasEvent) {
              if (isDropdown) {
                _handleDropdownEvent(*eventResult);
              } else {
                _handleMenuBarEvent(*eventResult);
              }
            }
          }

          free(message);

          if (isDropdown) {
            _requestDropdownEvent(dropdownEventPortID);
          } else {
            _requestMenuBarEvent(_eventReplyPortID);
          }

          continue;
        }
      } else {
        message = _kernel.ReceiveIPCMessage(_portHandle);
      }

      if (!message) continue;

      // dispatch context operations via the base Server machinery
      ProcessNextMessage(message);

      // flush menu bar if any handler set the refresh flag
      if (_contextController.NeedsRefresh) {
        _contextController.NeedsRefresh = false;
        _refreshMenuBar();
      }

      free(message);
    }
  }

  void ContextServer::_requestMenuBarEvent(
    IPCPortID eventReplyPortID
  ) {
    IPCPortResourceID sendHandle = _kernel.OpenIPCPort(
      AppServer::PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    AppServer::GetOverlayEventRequest request = {};

    request.ABIVersion = AppServer::Version;
    request.Operation = AppServer::Operation::GetOverlayEvent;
    request.ReplyPortID = eventReplyPortID;
    request.ID = _menuBar.GetWindowID();

    _kernel.SendIPCMessage(sendHandle, &request, sizeof(request));
    _kernel.CloseIPCPort(sendHandle);
  }

  void ContextServer::_handleMenuBarEvent(
    const AppServer::WindowEventResult& eventResult
  ) {
    switch (eventResult.Type) {
      case AppServer::WindowEventType::MouseDown: {
        ActionID action = _menuBar.HandleMouseDown(
          eventResult.MouseX,
          eventResult.MouseY,
          _menuService.GetMenuBarState()
        );

        if (action != 0) {
          // TODO: dispatch action via InvokeAction
        }

        if (
          _menuBar.IsDropdownOpen() &&
          _menuBar.GetDropdownWindowID() != 0
        ) {
          _requestDropdownEvent(static_cast<IPCPortID>(
            600 + _kernel.GetProcessID() * 3 + 1
          ));
        }

        break;
      }

      default:
        break;
    }
  }

  void ContextServer::_requestDropdownEvent(
    IPCPortID eventReplyPortID
  ) {
    UInt32 dropdownID = _menuBar.GetDropdownWindowID();

    if (dropdownID == 0) return;

    IPCPortResourceID sendHandle = _kernel.OpenIPCPort(
      AppServer::PortID,
      IPCPortRights::Send
    );

    if (sendHandle == static_cast<IPCPortResourceID>(-1)) return;

    AppServer::GetOverlayEventRequest request = {};

    request.ABIVersion = AppServer::Version;
    request.Operation = AppServer::Operation::GetOverlayEvent;
    request.ReplyPortID = eventReplyPortID;
    request.ID = dropdownID;

    _kernel.SendIPCMessage(sendHandle, &request, sizeof(request));
    _kernel.CloseIPCPort(sendHandle);
  }

  void ContextServer::_handleDropdownEvent(
    const AppServer::WindowEventResult& eventResult
  ) {
    const MenuBarState& state = _menuService.GetMenuBarState();

    switch (eventResult.Type) {
      case AppServer::WindowEventType::MouseDown: {
        if (eventResult.MouseX < 0 || eventResult.MouseY < 0) {
          if (_menuBar.IsDropdownOpen()) {
            _menuBar.CloseDropdown();
            _menuBar.Render(state);
          }
        }

        break;
      }

      case AppServer::WindowEventType::MouseUp: {
        ActionID action = _menuBar.HandleDropdownMouseDown(
          eventResult.MouseX,
          eventResult.MouseY,
          state
        );

        if (action != 0) {
          _dispatchAction(action);
        }

        break;
      }

      case AppServer::WindowEventType::MouseMove: {
        _menuBar.HandleDropdownMouseMove(
          eventResult.MouseX,
          eventResult.MouseY,
          state
        );

        break;
      }

      default:
        break;
    }
  }

  void ContextServer::_dispatchAction(ActionID action) {
    if (action == static_cast<ActionID>(WellKnownAction::About)) {
      _handleSystemAction(action);

      return;
    }

    if (
      action >= static_cast<ActionID>(WellKnownAction::SystemDefined) &&
      action < static_cast<ActionID>(WellKnownAction::AppDefined)
    ) {
      _handleSystemAction(action);

      return;
    }

    AppServer::DeliverMenuAction(0, action);
  }

  static void _launchFromDisk(const char* path, const char* name) {
    using namespace Quantum::Servers::FileSystem::ABI;

    Quantum::Clients::FileSystemClient fileSystem;

    FileSystemFileStat stat = {};

    if (!fileSystem.Stat(path, &stat) || stat.Size == 0) return;

    auto fileHandle = fileSystem.Open(
      path,
      static_cast<UInt32>(FileSystemOpenFlags::Read)
    );

    if (fileHandle == 0) return;

    UIntPtr buffer = AllocateBlock(stat.Size);

    if (buffer == 0) {
      fileSystem.Close(fileHandle);

      return;
    }

    Int32 bytesRead = fileSystem.Read(
      fileHandle,
      reinterpret_cast<void*>(buffer),
      stat.Size,
      0
    );

    fileSystem.Close(fileHandle);

    if (bytesRead <= 0) {
      FreeBlock(buffer);

      return;
    }

    Quantum::Clients::RunClient runClient;

    runClient.LoadELF(
      name,
      path,
      reinterpret_cast<const void*>(buffer),
      static_cast<Size>(bytesRead)
    );

    FreeBlock(buffer);
  }

  static void _launchAppThread(UInt32 argument) {
    if (argument == 0) {
      _launchFromDisk("QUANTUM/Apps/Terminal.qapp", "Terminal.qapp");
    } else if (argument == 1) {
      _launchFromDisk("QUANTUM/Apps/About.qapp", "About.qapp");
    }

    Quantum::Threading::Thread::Exit(0);
  }

  void ContextServer::_handleSystemAction(ActionID action) {
    UInt32 argument;

    if (action == static_cast<ActionID>(WellKnownAction::About)) {
      argument = 1;
    } else {
      argument
        = action - static_cast<ActionID>(WellKnownAction::SystemDefined);
    }

    Quantum::Threading::Thread::Create(_launchAppThread, argument);
  }

  void ContextServer::_registerSystemMenus() {
    Menu systemMenu = {};

    systemMenu.ID = 1;

    CString::Copy("System", systemMenu.Title, MaxMenuTitleLength);

    systemMenu.Position = 0;
    systemMenu.IsFocusMenu = true;

    MenuItem aboutItem = {};

    aboutItem.Action = static_cast<ActionID>(WellKnownAction::About);

    CString::Copy("About", aboutItem.Label, MaxMenuItemLabelLength);

    aboutItem.State = ActionState::Enabled;

    systemMenu.AddItem(aboutItem);

    MenuItem shellItem = {};

    shellItem.Action = static_cast<ActionID>(WellKnownAction::SystemDefined);

    CString::Copy("Terminal", shellItem.Label, MaxMenuItemLabelLength);

    shellItem.State = ActionState::Enabled;

    systemMenu.AddItem(shellItem);

    _menuService.AddSystemMenu(static_cast<Menu&&>(systemMenu));
  }

  void ContextServer::_refreshMenuBar() {
    const MenuBarState& state = _menuService.GetMenuBarState();

    if (!_menuBar.IsValid()) {
      if (!_menuBar.Create(1024)) return;

      if (_eventReplyPortID != 0 && !_eventListening) {
        _requestMenuBarEvent(_eventReplyPortID);
        _eventListening = true;
      }
    }

    _menuBar.Render(state);
  }
}

/**
 * @brief Entry point for @ref @QCtxSrv::ContextServer.
 * @return Non-zero error code on failure. Does **not** return on success.
 */
int Main() {
  KernelClient kernel;
  StartupClient startupClient;
  ServerLog log(kernel);
  MenuService menuService(kernel, log);
  MenuBar menuBar(kernel, log);

  ContextController contextController(
    kernel,
    log,
    menuService,
    menuBar
  );

  ContextServer server(
    startupClient,
    log,
    contextController,
    menuService,
    menuBar
  );

  return server.Run();
}
