/**
 * @file Servers/App/InputDispatcher.hpp
 * @brief Declares @ref @QAppSrv::InputDispatcher.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AppServerTypes.hpp>

#include "Windows/Window.hpp"

namespace Quantum::Servers::App {
  /**
   * @brief Routes mouse and keyboard events to the appropriate window or
   *        server subsystem.
   *
   * Encapsulates the drag, resize, button-press, mouse-capture, and
   * hover-cursor state machines previously inlined in @c Server::Start().
   * Each event type is dispatched to a dedicated private handler that
   * mutates internal state and delegates rendering / IPC through the
   * injected subsystem references.
   */
  class InputDispatcher {
    public:
      /**
       * @brief Constructs an input dispatcher with all required subsystem
       *        references.
       * @param graphics IPC bridge for the graphics server.
       * @param windowManager Window lifecycle and Z-order manager.
       * @param compositor Damage / compositing manager (forward-declared).
       * @param cursorManager Cursor position and bitmap manager.
       * @param screenWidth Logical screen width in pixels.
       * @param screenHeight Logical screen height in pixels.
       * @param hasHardwareCursor Whether the driver provides a hardware cursor.
       * @param hasFastScreenBlit Whether the driver provides HW screen BLT.
       * @param bufferBpp Bytes per pixel of the compositing buffer.
       */
      explicit InputDispatcher(
        KernelClient& kernel,
        GraphicsClient& graphics,
        WindowManager& windowManager,
        Compositor& compositor,
        CursorManager& cursorManager,
        UInt16 screenWidth,
        UInt16 screenHeight,
        bool hasHardwareCursor,
        bool hasFastScreenBlit,
        UInt8 bufferBpp
      );

      /**
       * @brief Destroys the input dispatcher instance.
       */
      virtual ~InputDispatcher() = default;

      /**
       * @brief Main entry point: routes an input event to the appropriate
       *        handler based on its type.
       * @param event The input event to dispatch.
       */
      void Dispatch(const InputEvent& event);

      /**
       * @brief Returns whether the dispatcher is in a drag or resize
       *        interaction that requires custom cursor management.
       */
      bool IsInteracting() const {
        return _dragging || _resizing;
      }

      /**
       * @brief Marks that the cursor position was already updated for
       *        the next mouse move event (early update before lock).
       */
      void SetCursorPreUpdated() { _cursorPreUpdated = true; }

      /**
       * @brief Sets the dock for click handling.
       */
      void SetDock(Dock::Dock* dock) { _dock = dock; }

      /**
       * @brief Retrieves the next input event from the input server,
       *        blocking until an event is available.
       * @param outEvent Pointer to an InputEvent structure to receive the
       *                 event data.
       * @return `true` if an event was successfully retrieved, `false` on
       *         error.
       */
      bool GetNextEvent(InputEvent* outEvent);

      /**
       * @brief Attempts to retrieve the next input event from the input
       *        server without blocking.
       * @param outEvent Pointer to an InputEvent structure to receive the
       *                 event data if available.
       * @return `true` if an event was successfully retrieved, `false` if
       *         no event is available or on error.
       */
      bool TryGetNextEvent(InputEvent* outEvent);

      /**
       * @brief Sends a `GetNextEvent` request to the input server, marking
       *        the request as pending.
       */
      void SendInputRequest();

      /**
       * @brief Returns whether a `GetNextEvent` request is currently
       *        pending at the input server.
       * @return `true` if a request is outstanding.
       */
      bool IsInputRequestPending() const;

      /**
       * @brief Sets the input-request-pending flag.
       * @param pending The new value.
       */
      void SetInputRequestPending(bool pending) {
        _inputRequestPending = pending;
      }

      /**
       * @brief Returns the IPC receive handle for input event replies,
       *        used by the multiplexed wait in the main event loop.
       * @return The input reply port resource handle.
       */
      IPCPortResourceID GetInputReplyHandle() const;

      /**
       * @brief Pointer to the owning server's `DebugFlushRegion` toggle,
       *        flipped on Escape key press.
       */
      bool* DebugFlushRegion = nullptr;

      /**
       * @brief Sets the overlay manager for input hit-testing.
       * @param overlayManager Pointer to the overlay manager.
       */
      void SetOverlayManager(OverlayManager* overlayManager) {
        _overlayManager = overlayManager;
      }

    private:
      /**
       * @brief Handles a mouse-move event: cursor position update, resize
       *        state machine, drag state machine, button hover tracking,
       *        mouse capture delivery, and hover cursor changes.
       * @param event The mouse-move input event.
       */
      void _handleMouseMove(const InputEvent& event);

      /**
       * @brief Handles a mouse-button-down event: modal redirect, close /
       *        maximize button press, bring-to-front, content-area click
       *        and mouse capture.
       * @param event The mouse-button-down input event.
       */
      void _handleMouseDown(const InputEvent& event);

      /**
       * @brief Handles a mouse-button-up event: button release (close /
       *        maximize), final resize event, drag reconcile, capture
       *        release.
       * @param event The mouse-button-up input event.
       */
      void _handleMouseUp(const InputEvent& event);

      /**
       * @brief Handles keyboard and scroll events: forwarding to the
       *        active window, and the Escape debug toggle.
       * @param event The keyboard or scroll input event.
       */
      void _handleKeyboardScroll(const InputEvent& event);

      /**
       * @brief Reference to the kernel client for IPC operations.
       */
      KernelClient& _kernel;

      /**
       * @brief IPC bridge for the graphics server.
       */
      GraphicsClient& _graphics;

      /**
       * @brief Window lifecycle and Z-order manager.
       */
      WindowManager& _windowManager;

      /**
       * @brief Overlay manager for input hit-testing.
       */
      OverlayManager* _overlayManager = nullptr;

      /**
       * @brief The overlay that last received a MouseMove event, used
       *        to deliver leave events when the cursor moves off.
       */
      Overlay* _lastHoveredOverlay = nullptr;

      /**
       * @brief Damage / compositing manager.
       */
      Compositor& _compositor;

      /**
       * @brief Cursor position and bitmap manager.
       */
      CursorManager& _cursorManager;

      /**
       * @brief Logical screen width in pixels.
       */
      UInt16 _screenWidth;

      /**
       * @brief Logical screen height in pixels.
       */
      UInt16 _screenHeight;

      /**
       * @brief Whether the active graphics driver provides a hardware
       *        cursor.
       */
      bool _hasHardwareCursor;

      /**
       * @brief Whether the active graphics driver provides hardware-
       *        accelerated screen-to-screen BLT.
       */
      bool _hasFastScreenBlit;

      /**
       * @brief Bytes per pixel of the compositing buffer (2 or 4).
       */
      UInt8 _bufferBpp;

      /**
       * @brief Whether the left mouse button is currently held down.
       */
      bool _leftButtonDown = false;

      /**
       * @brief The title bar button currently being held down, or
       *        `nullptr` if no button press is in progress.
       */
      WindowTitleBarButton* _pressedButton = nullptr;

      /**
       * @brief The window node owning the pressed button.
       */
      PathNode<Window*>* _pressedButtonNode = nullptr;

      /**
       * @brief Whether the user is currently dragging a window.
       */
      bool _dragging = false;

      /**
       * @brief The list node of the window currently being dragged.
       */
      PathNode<Window*>* _dragNode = nullptr;

      /**
       * @brief The offset from the mouse cursor to the top-left corner of
       *        the window being dragged.
       */
      Point _dragOffset;

      /**
       * @brief Whether the user is currently resizing a window.
       */
      bool _resizing = false;

      /**
       * @brief Set by the main loop when the cursor was already moved
       *        before acquiring the state lock (early update for
       *        responsiveness). Cleared after _handleMouseMove uses it.
       */
      bool _cursorPreUpdated = false;

      /**
       * @brief The dock taskbar for click handling.
       */
      Dock::Dock* _dock = nullptr;

      /**
       * @brief The list node of the window currently being resized.
       */
      PathNode<Window*>* _resizeNode = nullptr;

      /**
       * @brief The cursor position at the start of a resize drag.
       */
      Point _resizeStart;

      /**
       * @brief The width of the window at the start of a resize drag.
       */
      UInt16 _resizeStartWidth = 0;

      /**
       * @brief The height of the window at the start of a resize drag.
       */
      UInt16 _resizeStartHeight = 0;

      /**
       * @brief Index into the resource table of the window that captured
       *        the mouse, or -1 if no capture is active.
       */
      Size _mouseCaptureIndex = static_cast<Size>(-1);

      /**
       * @brief Whether the cursor is currently hovering over a resize
       *        handle. Used to avoid sending redundant bitmap-switch
       *        requests on every mouse-move event.
       */
      bool _hoverOverResize = false;

      /**
       * @brief IPC send handle for the input server port.
       */
      IPCPortResourceID _inputSendHandle;

      /**
       * @brief IPC receive handle for input event replies.
       */
      IPCPortResourceID _inputReplyHandle;

      /**
       * @brief Auto-assigned port ID for receiving input event replies.
       */
      Kernel::IPC::IPCPortID _inputReplyPortID;

      /**
       * @brief Whether a `GetNextEvent` request is currently pending at
       *        the input server.
       */
      bool _inputRequestPending = false;
  };
}
