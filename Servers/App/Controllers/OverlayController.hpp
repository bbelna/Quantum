/**
 * @file Servers/App/Controllers/OverlayController.hpp
 * @brief Declares @ref @QAppSrv::Controllers::OverlayController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "Overlays/OverlayManager.hpp"

namespace Quantum::Servers::App::Controllers {
  /**
   * @brief Handles all overlay management IPC requests from client
   *        applications.
   *
   * Owns the @ref OverlayManager and routes each overlay
   * @ref ABI::Operation to a dedicated private handler.
   */
  class OverlayController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref OverlayController.
       * @param kernel Reference to the @ref KernelClient.
       * @param log Reference to the @ref ServerLog.
       * @param compositor Damage / compositing manager.
       */
      OverlayController(
        KernelClient& kernel,
        ServerLog& log,
        Compositor& compositor
      );

      /**
       * @brief Dispatches an overlay management operation.
       * @param operation The operation code.
       * @param message The received IPC message.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

      /**
       * @brief Returns a reference to the overlay manager.
       */
      OverlayManager& GetOverlayManager() { return _overlayManager; }

    private:
      void _handleCreateOverlay(const IPCMessage* message);
      void _handleCloseOverlay(const IPCMessage* message);
      void _handleInvalidateOverlay(const IPCMessage* message);
      void _handleGetOverlayEvent(const IPCMessage* message);
      void _handleSetOverlayPosition(const IPCMessage* message);

      Compositor& _compositor;
      OverlayManager _overlayManager;
  };
}
