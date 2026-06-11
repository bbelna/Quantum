/**
 * @file Servers/Context/ContextController.hpp
 * @brief Declares @ref @QCtxSrv::ContextController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <ContextServerTypes.hpp>

#include "MenuService.hpp"
#include "MenuBar.hpp"

namespace Quantum::Servers::Context {
  /**
   * @brief Handles context operations: provider registration, contribution
   *        pushes, focus context updates, action invocations, and menu bar
   *        state queries.
   *
   * After handling operations that modify menu state, sets
   * @ref NeedsRefresh so the server's main loop can coalesce and flush
   * the menu bar rendering.
   */
  class ContextController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref ContextController.
       * @param kernel Reference to the @ref KernelClient for IPC
       *               operations. Must outlive this controller.
       * @param log Reference to the @ref ServerLog for logging. Must
       *            outlive this controller.
       * @param menuService Reference to the @ref MenuService for menu
       *                    operations. Must outlive this controller.
       * @param menuBar Reference to the @ref MenuBar for refresh
       *                signaling. Must outlive this controller.
       */
      ContextController(
        KernelClient& kernel,
        ServerLog& log,
        MenuService& menuService,
        MenuBar& menuBar
      );

      /**
       * @brief Dispatches a context operation to the appropriate handler.
       * @param operation The operation code.
       * @param message The received @ref IPCMessage.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

      /**
       * @brief Whether the menu bar needs re-rendering after the last
       *        handled operation. The server loop checks and clears this.
       */
      bool NeedsRefresh = false;

    private:
      MenuService& _menuService;
      MenuBar& _menuBar;

      void _handleRegisterProvider(const IPCMessage* message);
      void _handleUnregisterProvider(const IPCMessage* message);
      void _handleSetContributions(const IPCMessage* message);
      void _handleUpdateFocusContext(const IPCMessage* message);
      void _handleInvokeAction(const IPCMessage* message);
      void _handleGetMenuBarState(const IPCMessage* message);
  };
}
