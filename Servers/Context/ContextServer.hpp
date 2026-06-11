/**
 * @file Servers/Context/ContextServer.hpp
 * @brief Declares @ref @QCtxSrv::ContextServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <ContextServerTypes.hpp>

#include "ContextController.hpp"
#include "MenuBar.hpp"
#include "MenuService.hpp"

namespace Quantum::Servers::Context {
  /**
   * @brief The context server: manages the system's focus context and the
   *        composed menu bar.
   *
   * Registers a @ref ContextController for IPC operations. Overrides
   * @ref Run to multiplex between the main context port and window event
   * ports from the application server.
   */
  class ContextServer : public Server {
    public:
      /**
       * @brief Creates a new @ref ContextServer.
       * @param startupClient Reference to the @ref StartupClient. Must
       *                      outlive this server instance.
       * @param log Reference to the @ref ServerLog. Must outlive this
       *            server instance.
       * @param contextController Reference to the @ref ContextController.
       *                          Must outlive this instance.
       * @param menuService Reference to the @ref MenuService. Must outlive
       *                    this server instance.
       * @param menuBar Reference to the @ref MenuBar. Must outlive this
       *                server instance.
       */
      ContextServer(
        StartupClient& startupClient,
        ServerLog& log,
        ContextController& contextController,
        MenuService& menuService,
        MenuBar& menuBar
      );

      /**
       * @brief Enters the server main loop. Overrides @ref Server::Run to
       *        multiplex between the context IPC port and window event
       *        ports from the application server.
       * @return Non-zero termination code on failure.
       */
      Int32 Run();

    private:
      /**
       * @brief Kernel client for this server's direct kernel operations.
       *
       * Declared first so later members are initialized after it.
       */
      KernelClient _kernel;

      ServerLog& _log;
      ContextController& _contextController;
      MenuService& _menuService;
      MenuBar& _menuBar;

      /**
       * @brief The event reply port ID used for GetWindowEvent on the
       *        menu bar window.
       */
      IPCPortID _eventReplyPortID = 0;

      /**
       * @brief Whether the initial GetWindowEvent has been sent.
       */
      bool _eventListening = false;

      /**
       * @brief Refreshes the menu bar rendering and starts event listening
       *        if needed.
       */
      void _refreshMenuBar();

      /**
       * @brief Registers the persistent System menu.
       */
      void _registerSystemMenus();

      /**
       * @brief Dispatches an action, system actions are handled locally,
       *        app actions are delivered to the focused window.
       * @param action The action ID to dispatch.
       */
      void _dispatchAction(ActionID action);

      /**
       * @brief Handles a system action (launch app, settings, etc.).
       * @param action The system action ID.
       */
      void _handleSystemAction(ActionID action);

      /**
       * @brief Handles a window event from the menu bar.
       * @param eventResult The window event result from the AppServer.
       */
      void _handleMenuBarEvent(
        const AppServer::WindowEventResult& eventResult
      );

      /**
       * @brief Handles a window event from the dropdown panel.
       * @param eventResult The window event result from the AppServer.
       */
      void _handleDropdownEvent(
        const AppServer::WindowEventResult& eventResult
      );

      /**
       * @brief Sends a GetWindowEvent request for the menu bar window.
       * @param eventReplyPortID The port ID to receive the reply on.
       */
      void _requestMenuBarEvent(IPCPortID eventReplyPortID);

      /**
       * @brief Sends a GetWindowEvent request for the dropdown window.
       * @param eventReplyPortID The port ID to receive the reply on.
       */
      void _requestDropdownEvent(IPCPortID eventReplyPortID);
  };
}
