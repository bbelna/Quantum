/**
 * @file App/QuantumOSWindow.hpp
 * @brief Declares @ref @QApp::OS::QuantumOS::OSWindow.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/App/OS/IOSWindow.hpp>

#include "QuantumOSAppTypes.hpp"

/**
 * @brief QuantumOS-specific window backend.
 */
namespace Quantum::App::OS::QuantumOS {
  /**
   * @brief QuantumOS implementation of @ref @QApp::OS::IOSWindow.
   */
  class OSWindow : public IOSWindow {
    public:
      /**
       * @brief Creates the platform window.
       * @param x The x-coordinate of the window's top-left corner.
       * @param y The y-coordinate of the window's top-left corner.
       * @param width The width of the window in pixels.
       * @param height The height of the window in pixels.
       * @param title The null-terminated title string.
       * @param canvas Pointer to the canvas to initialize with the content
       *                buffer.
       * @param contentColor The content area background color.
       * @param maximized If true, the window is created maximized.
       * @param allowClose If true, the window has a close button.
       * @param allowMaximize If true, the window has a maximize button.
       * @return `true` if the window was created successfully.
       */
      bool Create(
        Int16 x,
        Int16 y,
        UInt16 width,
        UInt16 height,
        const char* title,
        UI::Canvas* canvas,
        UInt32 contentColor,
        bool maximized,
        bool allowClose,
        bool allowMaximize,
        bool allowResize,
        UInt32 innerContentColor = 0
      );

      /**
       * @brief Destroys the platform window.
       */
      void Destroy();

      /**
       * @brief Sets the position of the window.
       * @param x The new x-coordinate.
       * @param y The new y-coordinate.
       */
      void SetPosition(Int16 x, Int16 y);

      /**
       * @brief Sets the size of the window.
       * @param width The new width in pixels.
       * @param height The new height in pixels.
       */
      void SetSize(UInt16 width, UInt16 height);

      /**
       * @brief Marks the window's content area as dirty.
       */
      void Invalidate();

      /**
       * @brief Marks a sub-region of the window's content area as dirty.
       * @param dirtyRectangle The dirty rectangle in content-pixel coordinates.
       */
      void Invalidate(Rectangle dirtyRectangle);

      /**
       * @brief Marks a sub-region as dirty with a pixel-shift hint.
       * @param shiftDeltaY Signed pixel shift (positive = up).
       * @param dirtyRectangle The dirty strip in content-pixel coordinates.
       */
      void InvalidateWithShift(
        Int16 shiftDeltaY,
        Rectangle dirtyRectangle
      );

      /**
       * @brief Blocks until a window event is available.
       * @param outEvent Pointer to receive the event.
       * @param canvas Pointer to the canvas, updated on resize.
       * @return `true` if an event was received; `false` on failure.
       */
      bool GetWindowEvent(WindowEvent* outEvent, UI::Canvas* canvas);

      /**
       * @brief Non-blocking variant of @ref GetWindowEvent. Returns
       *        immediately with `false` when no events are queued.
       * @param outEvent Pointer to receive the event.
       * @param canvas Pointer to the canvas, updated on resize.
       * @return `true` if an event was received; `false` if no event
       *         is pending or on failure.
       */
      bool TryGetWindowEvent(WindowEvent* outEvent, UI::Canvas* canvas);

      /**
       * @brief Sets the minimum allowed size for this window.
       * @param minWidth The minimum width in pixels.
       * @param minHeight The minimum height in pixels.
       */
      void SetMinimumSize(UInt16 minWidth, UInt16 minHeight);

      /**
       * @brief Blocks until the user clicks the window's close button.
       */
      void WaitForClose();

      /**
       * @brief Returns the server-assigned resource ID.
       */
      UInt32 GetResourceID() const { return _id; }

    private:
      AppServer::WindowResourceID _id = 0;

      UInt32 _contentBufferAddress = 0;

      SharedBufferID _contentBufferID = 0;

      UInt32 _contentBufferCapacity = 0;

      SharedBufferID _pendingContentBufferID = 0;

      UInt16 _pendingContentWidth = 0;

      UInt16 _pendingContentHeight = 0;

      UInt16 _pendingContentStride = 0;

      UInt8 _pendingContentBPP = 0;
  };
}
