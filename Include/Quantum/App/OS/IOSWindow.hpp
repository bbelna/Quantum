/**
 * @file Include/Quantum/App/IOSWindow.hpp
 * @brief Defines @ref Quantum::App::OS::IOSWindow.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/App/WindowEvent.hpp>
#include <Quantum/Core/Types.hpp>
#include <Quantum/Geometry2D.hpp>
#include <Quantum/UI.hpp>

/**
 * @brief Interface and implementations for OS-specific window backends.
 */
namespace Quantum::App::OS {
  /**
   * @brief Interface for OS-specific window backends.
   */
  class IOSWindow {
    public:
      /**
       * @brief Creates the OS window.
       * @param x The x-coordinate of this window's top-left corner.
       * @param y The y-coordinate of this window's top-left corner.
       * @param width The width of this window in pixels.
       * @param height The height of this window in pixels.
       * @param title The null-terminated title string.
       * @param canvas Pointer to the canvas to initialize with the content
       *                buffer.
       * @param contentColor The content area background color.
       * @param maximized If `true`, this window is created maximized.
       * @param allowClose If `true`, this window has a close button.
       * @param allowMaximize If `true`, this window has a maximize button.
       * @param allowResize If `true`, this window is resizable.
       * @return `true` if this window was created successfully; `false` on
       *         failure.
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
        bool allowResize
      );

      /**
       * @brief Destroys this OS window implementation, releasing all resources.
       */
      void Destroy();

      /**
       * @brief Sets the position of this window.
       * @param x The new x-coordinate.
       * @param y The new y-coordinate.
       */
      void SetPosition(Int16 x, Int16 y);

      /**
       * @brief Sets the size of this window.
       * @param width The new width in pixels.
       * @param height The new height in pixels.
       */
      void SetSize(UInt16 width, UInt16 height);

      /**
       * @brief Marks this window's content area as dirty.
       */
      void Invalidate();

      /**
       * @brief Marks a sub-region of this window's content area as dirty.
       * @param dirtyRect The dirty rectangle in content-pixel coordinates.
       */
      void Invalidate(Geometry2D::Rectangle dirtyRect);

      /**
       * @brief Marks a sub-region as dirty with a pixel-shift hint.
       * @param shiftDeltaY Signed pixel shift (positive = up).
       * @param dirtyRect The dirty strip in content-pixel coordinates.
       */
      void InvalidateWithShift(
        Int16 shiftDeltaY,
        Geometry2D::Rectangle dirtyRect
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
       * @brief Blocks until the user clicks this window's close button.
       */
      void WaitForClose();
  };
}
