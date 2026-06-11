/**
 * @file Servers/App/Controllers/WindowController.hpp
 * @brief Declares @ref @QAppSrv::Controllers::WindowController.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "Windows/DefaultWindowTheme.hpp"

namespace Quantum::Servers::App::Controllers {
  /**
   * @brief Handles all window management IPC requests from client
   *        applications.
   *
   * Routes each @ref ABI::Operation to a dedicated private handler.
   * All mutable state is accessed through the injected @ref WindowManager,
   * @ref Compositor, and @ref GraphicsClient references.
   */
  class WindowController : public RequestController {
    public:
      /**
       * @brief Creates a new @ref WindowController.
       * @param kernel Reference to the @ref KernelClient.
       * @param log Reference to the @ref ServerLog.
       * @param windowManager Window lifecycle and Z-order manager.
       * @param compositor Damage / compositing manager.
       * @param graphics IPC bridge for the graphics server.
       * @param overlayManager Overlay manager (for menu bar height).
       * @param screenWidth Logical screen width in pixels.
       * @param screenHeight Logical screen height in pixels.
       * @param bufferBPP Bytes per pixel of the compositing buffer.
       * @param hasHardwareCursor Whether the driver provides a HW cursor.
       * @param hasFastScreenBlit Whether the driver provides HW screen BLT.
       * @param primaryFontBufferID SharedBufferID for the primary font.
       * @param primaryFontDataSizeInBytes Size of the primary font data.
       * @param secondaryFontBufferID SharedBufferID for the secondary font.
       * @param secondaryFontDataSizeInBytes Size of the secondary font data.
       */
      WindowController(
        KernelClient& kernel,
        ServerLog& log,
        WindowManager& windowManager,
        Compositor& compositor,
        GraphicsClient& graphics,
        OverlayManager& overlayManager,
        UInt16 screenWidth,
        UInt16 screenHeight,
        UInt8 bufferBPP,
        bool hasHardwareCursor,
        bool hasFastScreenBlit,
        SharedBufferID primaryFontBufferID,
        UInt32 primaryFontDataSizeInBytes,
        SharedBufferID secondaryFontBufferID,
        UInt32 secondaryFontDataSizeInBytes
      );

      /**
       * @brief Dispatches a window management operation.
       * @param operation The operation code.
       * @param message The received IPC message.
       */
      void Handle(UInt32 operation, const IPCMessage* message) override;

      /**
       * @brief Returns and clears the dock-dirty flag.
       *
       * Operations such as CreateWindow, CloseWindow, and SetModal set
       * this flag to signal that the dock needs rebuilding.
       *
       * @return `true` if the dock is dirty since the last call.
       */
      bool ConsumeDockDirty() {
        bool dirty = _dockDirty;
        _dockDirty = false;
        return dirty;
      }

    private:
      void _handleCreateWindow(const IPCMessage* message);
      void _handleCloseWindow(const IPCMessage* message);
      void _handleSetPosition(const IPCMessage* message);
      void _handleSetSize(const IPCMessage* message);
      void _handleWaitForClose(const IPCMessage* message);
      void _handleInvalidateContent(const IPCMessage* message);
      void _handleInvalidateContentShift(const IPCMessage* message);
      void _handleGetWindowEvent(const IPCMessage* message);
      void _handleTryGetWindowEvent(const IPCMessage* message);
      void _handleSetMinimumSize(const IPCMessage* message);
      void _handleSetModal(const IPCMessage* message);
      void _handleDeliverMenuAction(const IPCMessage* message);
      void _handleGetSystemFonts(const IPCMessage* message);

      WindowManager& _windows;
      Compositor& _compositor;
      GraphicsClient& _graphics;
      OverlayManager& _overlayManager;

      UInt16 _screenWidth;
      UInt16 _screenHeight;
      UInt8 _bufferBPP;
      bool _hasHardwareCursor;
      bool _hasFastScreenBlit;

      SharedBufferID _primaryFontBufferID;
      UInt32 _primaryFontDataSizeInBytes;
      SharedBufferID _secondaryFontBufferID;
      UInt32 _secondaryFontDataSizeInBytes;

      bool _dockDirty = false;

      Windows::WindowTheme _windowTheme = Windows::DefaultWindowTheme();
  };
}
