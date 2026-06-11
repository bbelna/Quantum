/**
 * @file Include/Quantum/App/WindowEvent.hpp
 * @brief Declares @ref @QApp::WindowEvent and related types.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Input.hpp>
#include <Quantum/Core/Types.hpp>

namespace Quantum::App {
  /**
   * @brief The type of event delivered by `Window::GetWindowEvent`.
   */
  enum class WindowEventType : UInt8 {
    /**
     * @brief A keyboard input event.
     */
    Keyboard = 1,

    /**
     * @brief The window was resized by the user. The new content
     *        dimensions are provided so the client can re-layout.
     */
    Resize = 2,

    /**
     * @brief A mouse scroll wheel event directed at this window.
     */
    MouseScroll = 3,

    /**
     * @brief The window has been closed by the user. The client should
     *        clean up and exit.
     */
    Close = 4,

    /**
     * @brief A mouse button was pressed inside the window's content area.
     */
    MouseDown = 5,

    /**
     * @brief A mouse button was released.
     */
    MouseUp = 6,

    /**
     * @brief The mouse cursor moved while a button is held.
     */
    MouseMove = 7,

    /**
     * @brief The window lost focus (another window became active).
     */
    Deactivated = 8,

    /**
     * @brief A menu action was invoked by the user (via the system menu
     *        bar or a keyboard shortcut). The action ID is in
     *        @ref WindowEvent::MenuActionID.
     */
    MenuAction = 9
  };

  /**
   * @brief A window event delivered to the application.
   */
  struct WindowEvent {
    /**
     * @brief The type of window event.
     */
    WindowEventType Type;

    /**
     * @brief The keyboard input event. Valid only when
     *        `Type == WindowEventType::Keyboard`.
     */
    Input::InputEvent KeyboardEvent;

    /**
     * @brief New content width in pixels. Valid only when
     *        `Type == WindowEventType::Resize`.
     */
    UInt16 ContentWidth;

    /**
     * @brief New content height in pixels. Valid only when
     *        `Type == WindowEventType::Resize`.
     */
    UInt16 ContentHeight;

    /**
     * @brief Scroll wheel delta. Positive = scroll up, negative =
     *        scroll down. Valid only when
     *        `Type == WindowEventType::MouseScroll`.
     */
    Int8 ScrollDelta;

    /**
     * @brief Content-relative X coordinate of the mouse cursor. Valid for
     *        MouseDown, MouseUp, and MouseMove events.
     */
    Int16 MouseX;

    /**
     * @brief Content-relative Y coordinate of the mouse cursor. Valid for
     *        MouseDown, MouseUp, and MouseMove events.
     */
    Int16 MouseY;

    /**
     * @brief Mouse button flags (see `Input::MouseButton`). Valid for
     *        MouseDown, MouseUp, and MouseMove events.
     */
    UInt8 MouseButtons;

    /**
     * @brief The action ID invoked from the menu bar. Valid only when
     *        `Type == WindowEventType::MenuAction`.
     */
    UInt32 MenuActionID;
  };
}
