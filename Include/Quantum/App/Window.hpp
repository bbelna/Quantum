/**
 * @file Include/Quantum/App/Window.hpp
 * @brief Declares @ref @QApp::Window.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/App/WindowEvent.hpp>
#include <Quantum/Core/Types.hpp>
#include <Quantum/UI/Element.hpp>
#include <Quantum/UI/Canvas.hpp>

#include "OS/IOSWindow.hpp"

#if defined(OS_QUANTUMOS)
#include "../../../App/OS/QuantumOS/OSWindow.hpp"
#endif

namespace Quantum::App {
#if defined(OS_QUANTUMOS)
  using PlatformWindow = OS::QuantumOS::OSWindow;
#else
  class PlatformWindow;
#endif
  /**
   * @brief A window managed by the platform's window system. The constructor
   *        creates the window; the destructor closes it.
   */
  class Window {
    public:
      /**
       * @brief Creates a window.
       * @param x The x-coordinate of the window's top-left corner.
       * @param y The y-coordinate of the window's top-left corner.
       * @param width The width of the window in pixels.
       * @param height The height of the window in pixels.
       * @param title The null-terminated title string.
       * @param contentColor The content area background color.
       * @param maximized If true, the window is created maximized.
       * @param allowClose If true, the window has a close button.
       * @param allowMaximize If true, the window has a maximize button.
       */
      Window(
        Int16 x,
        Int16 y,
        UInt16 width,
        UInt16 height,
        const char* title,
        UInt32 contentColor = 0xFFFFFFFF,
        bool maximized = false,
        bool allowClose = true,
        bool allowMaximize = true,
        bool allowResize = true,
        UInt32 innerContentColor = 0
      );

      /**
       * @brief Closes the window.
       */
      ~Window();

      /**
       * @brief Returns whether the window was successfully created.
       * @return `true` if the window is valid; `false` otherwise.
       */
      bool IsValid() const { return _valid; }

      /**
       * @brief Returns the content area background color this window
       *        was created with.
       *
       * Used by event-loop hooks (e.g. resize handlers) that need to
       * clear the surface to its configured background.
       */
      UInt32 GetContentColor() const { return _contentColor; }

      /**
       * @brief Returns the server-assigned resource ID for this window.
       *        Used to establish modal relationships between windows.
       */
      UInt32 GetResourceID() const;

      /**
       * @brief Sets the position of the window.
       * @param x The new x-coordinate of the window's top-left corner.
       * @param y The new y-coordinate of the window's top-left corner.
       */
      void SetPosition(Int16 x, Int16 y);

      /**
       * @brief Sets the size of the window.
       * @param width The new width of the window in pixels.
       * @param height The new height of the window in pixels.
       */
      void SetSize(UInt16 width, UInt16 height);

      /**
       * @brief Returns the Canvas wrapping the window's content buffer.
       *        The caller renders into this surface, then calls
       *        `Invalidate()` to trigger a redraw.
       * @return Reference to the window's content surface.
       */
      UI::Canvas& GetCanvas() { return _canvas; }

      /**
       * @brief Marks the window's content area as dirty, causing the
       *        compositor to redraw.
       */
      void Invalidate();

      /**
       * @brief Marks a sub-region of the window's content area as dirty.
       * @param dirtyRect The dirty rectangle in content-pixel coordinates.
       */
      void Invalidate(Geometry2D::Rectangle dirtyRect);

      /**
       * @brief Marks a sub-region as dirty with a pixel-shift hint. The
       *        compositor may use the shift delta to move existing pixels
       *        via GPU blit instead of re-compositing the entire area.
       * @param shiftDeltaY Signed pixel shift (positive = up).
       * @param dirtyRect The dirty strip in content-pixel coordinates.
       */
      void InvalidateWithShift(
        Int16 shiftDeltaY, Geometry2D::Rectangle dirtyRect
      );

      /**
       * @brief Blocks until a window event is available for this window.
       * @param outEvent Pointer to receive the event.
       * @return `true` if an event was received; `false` on failure.
       */
      bool GetWindowEvent(WindowEvent* outEvent);

      /**
       * @brief Non-blocking variant of @ref GetWindowEvent. Returns
       *        immediately with `false` when no events are queued.
       * @param outEvent Pointer to receive the event.
       * @return `true` if an event was received; `false` if no event
       *         is pending or on failure.
       */
      bool TryGetWindowEvent(WindowEvent* outEvent);

      /**
       * @brief Sets the minimum allowed size for this window.
       * @param minWidth The minimum width in pixels.
       * @param minHeight The minimum height in pixels.
       */
      void SetMinimumSize(UInt16 minWidth, UInt16 minHeight);

      /**
       * @brief Registers a UI element for automatic event routing and
       *        drawing. The element is prepended to the window's intrusive
       *        linked list and its back-pointer is set.
       * @param element The element to add.
       */
      void Add(UI::Element& element);

      /**
       * @brief Removes a previously added UI element from the window's
       *        element list. Clears focus and capture if the element held
       *        either.
       * @param element The element to remove.
       */
      void Remove(UI::Element& element);

      /**
       * @brief Routes a window event to the appropriate registered element.
       *
       * Mouse events are dispatched to click targets via hit testing.
       * Keyboard events are forwarded to the focused element. Unhandled
       * event types (Close, Resize, etc.) return `false` so the app can
       * process them.
       * @param event The window event to dispatch.
       * @return `true` if an element consumed the event; `false` otherwise.
       */
      bool DispatchEvent(const WindowEvent& event);

      /**
       * @brief Draws all registered elements by walking the linked list
       *        and calling `Draw()` on each.
       */
      void DrawAll();

      /**
       * @brief Sets keyboard focus to the given element, unfocusing any
       *        previously focused element. Both elements receive
       *        @ref UI::Element::OnFocusChanged callbacks.
       * @param element The element to focus.
       */
      void SetFocus(UI::Element& element);

      /**
       * @brief Clears keyboard focus from the currently focused element,
       *        calling its @ref UI::Element::OnFocusChanged callback.
       *        If a default focus element is set, focus returns to it
       *        instead of clearing entirely.
       */
      void ClearFocus();

      /**
       * @brief Sets the default focus element. When focus would otherwise
       *        be cleared (e.g. clicking empty space or a non-keyboard
       *        element), focus returns to this element instead.
       * @param element The element to use as the default focus target.
       */
      void SetDefaultFocus(UI::Element& element);

      /**
       * @brief Blocks until the user clicks the window's close button.
       *
       * After this returns, the window has been destroyed on the server
       * side. The caller should clean up and exit.
       */
      void WaitForClose();

    private:
      /**
       * @brief Whether the window was successfully created.
       */
      bool _valid = false;

      /**
       * @brief The content area background color this window was
       *        created with.
       */
      UInt32 _contentColor = 0;

      /**
       * @brief The rendering surface backed by the content buffer.
       */
      UI::Canvas _canvas;

      /**
       * @brief Platform-specific window backend.
       */
      PlatformWindow _os;

      /**
       * @brief Head of the intrusive linked list of registered elements.
       */
      UI::Element* _firstElement = nullptr;

      /**
       * @brief The element that currently has keyboard focus, or `nullptr`.
       */
      UI::Element* _focused = nullptr;

      /**
       * @brief The element whose click target currently has mouse capture
       *        (between mouse-down and mouse-up), or `nullptr`.
       */
      UI::Element* _capturedElement = nullptr;

      /**
       * @brief The element that receives focus when no other element
       *        claims it, or `nullptr` for no default.
       */
      UI::Element* _defaultFocus = nullptr;
  };
}
