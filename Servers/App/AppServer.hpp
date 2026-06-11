/**
 * @file Servers/App/AppServer.hpp
 * @brief Declares @ref @QAppSrv::AppServer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "Compositor.hpp"
#include "Cursor/CursorManager.hpp"
#include "InputDispatcher.hpp"
#include "Windows/WindowManager.hpp"

namespace Quantum::Servers::App {
  /**
   * @brief Quantum's window manager and compositor.
   *
   * Extends the @ref Server base class for IPC port management and
   * controller dispatch. Owns and coordinates all subsystem classes that
   * together provide window management, input dispatch, compositing, and
   * graphics server communication.
   *
   * The server overrides the standard @ref Server::Run loop with a custom
   * @ref Start method that multiplexes between input server events and
   * client application requests.
   */
  class AppServer : public Server {
    public:
      /**
       * @brief Creates a new @ref AppServer instance.
       * @param log Reference to the server log (must outlive this server).
       */
      AppServer(ServerLog& log);

      /**
       * @brief Destroys this @ref AppServer instance.
       */
      virtual ~AppServer() = default;

      /**
       * @brief Starts the application server's main loop. Never returns.
       */
      void Start();

    private:
      /**
       * @brief Kernel client for this server's direct kernel operations.
       *
       * Declared first so later members that reference it during
       * construction are initialized after it.
       */
      KernelClient _kernel;

      /**
       * @brief Server log for structured logging through the kernel client.
       */
      ServerLog& _log;

      /**
       * @brief Mutex protecting server state during request handling and
       *        rendering. Acquired by the main thread before dispatching
       *        to any subsystem, and by the render thread during compositing.
       */
      Mutex _stateLock;

      /**
       * @brief The display scale factor reported by the graphics server.
       */
      UInt8 _scaleFactor = 1;

      /**
       * @brief Logical screen width in pixels.
       */
      UInt16 _screenWidth = 0;

      /**
       * @brief Logical screen height in pixels.
       */
      UInt16 _screenHeight = 0;

      /**
       * @brief Whether the active graphics driver provides a hardware cursor.
       */
      bool _hasHardwareCursor = false;

      /**
       * @brief Whether the active graphics driver supports fast screen BLT.
       */
      bool _hasFastScreenBlit = false;

      /**
       * @brief Raw font file buffer (owned, kept alive for process lifetime).
       */
      UIntPtr _titleFontBuffer = 0;

      /**
       * @brief Size of the font file data in bytes.
       */
      UInt32 _primaryFontDataSizeInBytes = 0;

      /**
       * @brief Shared buffer ID for serving the font data to client apps.
       */
      SharedBufferID _primaryFontBufferID = 0;

      /**
       * @brief Parsed title bar font (points into `_titleFontBuffer`).
       */
      BitmapFont _titleFont = {};

      /**
       * @brief Loads the title bar font from the file system, creates a
       *        shared buffer for it, and sets it as the active window
       *        title font.
       */
      void _loadTitleFont();

      /**
       * @brief Heap address of the default UI font data.
       */
      UIntPtr _defaultFontBuffer = 0;

      /**
       * @brief Size of the default UI font data in bytes.
       */
      UInt32 _secondaryFontDataSizeInBytes = 0;

      /**
       * @brief Shared buffer ID for serving the default font to apps.
       */
      SharedBufferID _secondaryFontBufferID = 0;

      /**
       * @brief Parsed default UI font (points into `_defaultFontBuffer`).
       */
      BitmapFont _defaultFont = {};

      /**
       * @brief Loads the default UI font from the file system.
       */
      void _loadDefaultFont();

      /**
       * @brief Graphics server IPC bridge.
       */
      GraphicsClient* _graphics = nullptr;

      /**
       * @brief Cursor position and bitmap manager.
       */
      CursorManager* _cursor = nullptr;

      /**
       * @brief Window lifecycle, Z-order, and focus manager.
       */
      WindowManager _windowManager;

      /**
       * @brief Buffer compositing and render pipeline.
       */
      Compositor _compositor;

      /**
       * @brief Input event routing and interaction state machines.
       */
      InputDispatcher* _input = nullptr;

      /**
       * @brief Window management request controller.
       */
      Controllers::WindowController* _windowController = nullptr;

      /**
       * @brief Overlay management request controller.
       */
      Controllers::OverlayController* _overlayController = nullptr;
  };
}
