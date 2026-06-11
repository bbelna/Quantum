/**
 * @file Include/Quantum/App/App.hpp
 * @brief Declares @ref @QApp::App.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>
#include <Quantum/Fonts/BitmapFont.hpp>
#include <Quantum/Theme.hpp>

#include "Window.hpp"

/**
 * @brief Generates a @c Main entry point that constructs an instance of
 *        @p AppClass and calls @ref Quantum::App::App::Run.
 * @param AppClass Fully-qualified (or in-scope) name of an
 *        @ref Quantum::App::App subclass.
 */
#define QUANTUM_APP(AppClass) \
  int Main() { \
    AppClass app; \
    \
    return app.Run(); \
  }

namespace Quantum::App {
  /**
   * @brief Process-level wrapper for a Quantum app.
   *
   * @ref App owns process-wide resources (system fonts, app server
   * connection state) and provides an event loop that pumps events for
   * any number of @ref Window instances registered via @ref AddWindow.
   *
   * Apps are not required to own a window. A subclass that needs a UI
   * constructs its own @ref Window members and registers them with the
   * base @ref App; an app with no UI (a service-style background app)
   * can simply override @ref Run.
   *
   * Typical usage: subclass @ref App, declare any @ref Window members,
   * register them via @ref AddWindow in the subclass constructor (or in
   * @ref Init), override @ref Init to add UI elements and set up state,
   * override the per-window event hooks as needed, then call @ref Run
   * from main.
   */
  class App {
    public:
      /**
       * @brief Maximum number of windows a single @ref App can manage.
       */
      static constexpr Size MaxWindows = 8;

      /**
       * @brief Static copy of the parsed system font (shared by all apps
       *        in the same process).
       */
      static inline Fonts::BitmapFont SystemFont = {};

      /**
       * @brief Static copy of the primary font.
       */
      static inline Fonts::BitmapFont PrimaryFont = {};

      /**
       * @brief Whether the primary font has been loaded.
       */
      static inline bool PrimaryFontLoaded = false;

      /**
       * @brief Static shared buffer address for the font data.
       */
      static inline UIntPtr SharedFontBuffer = 0;

      /**
       * @brief Whether the system font has been loaded.
       */
      static inline bool SystemFontLoaded = false;

      /**
       * @brief Constructs an @ref App and loads process-wide resources.
       *
       * The system font is loaded once per process and installed as the
       * @ref UI::Canvas default font, so any @ref Window constructed
       * after the @ref App base subobject will pick the font up
       * automatically. Subclasses should declare their @ref Window
       * members so that they are constructed after this base, then call
       * @ref AddWindow to register them for event pumping.
       */
      App();

      /**
       * @brief Destroys the @ref App.
       */
      virtual ~App();

      /**
       * @brief Registers a @ref Window with the @ref App event loop.
       * @param window The window to register. The caller retains
       *        ownership; the window must outlive the @ref App or be
       *        unregistered via @ref RemoveWindow before destruction.
       *
       * Events for registered windows are pumped by the default
       * @ref Run loop and routed to the appropriate per-window event
       * hook on the subclass.
       */
      void AddWindow(Window& window);

      /**
       * @brief Unregisters a previously added @ref Window.
       * @param window The window to unregister.
       *
       * Safe to call from inside an event hook. After this returns the
       * @ref App will no longer pump events for the window. The window
       * itself is not destroyed.
       */
      void RemoveWindow(Window& window);

      /**
       * @brief Returns the number of registered windows.
       */
      Size GetWindowCount() const { return _windowCount; }

      /**
       * @brief Returns the registered window at the given index.
       * @param index Index in the range `[0, GetWindowCount())`.
       */
      Window& GetWindow(Size index) const { return *_windows[index]; }

      /**
       * @brief Runs the application's event loop.
       * @return Exit code (`0` on success, negative on error).
       *
       * The default implementation:
       *   1. Calls @ref Init.
       *   2. For each registered window, performs an initial
       *      @ref Window::DrawAll and @ref Window::Invalidate.
       *   3. Calls @ref OnReady.
       *   4. Polls events from each registered window in a round-robin
       *      loop. For each event, the window's @ref Window::DispatchEvent
       *      is invoked first; events not consumed by elements are
       *      forwarded to the appropriate per-window virtual hook
       *      (@ref OnClose, @ref OnResize, etc.). The loop yields when
       *      no events are pending.
       *   5. Returns when @ref RequestExit has been called.
       *
       * Subclasses that need full control over the event loop may
       * override this method entirely.
       */
      virtual int Run();

      /**
       * @brief Signals the @ref App event loop to exit after the current
       *        iteration.
       */
      void RequestExit() { _running = false; }

      /**
       * @brief Returns whether the @ref App event loop is still running.
       */
      bool IsRunning() const { return _running; }

    protected:
      /**
       * @brief Called at the start of @ref Run before the event loop
       *        begins.
       *
       * Override to register windows, add UI elements, register
       * callbacks, and perform any other one-time setup.
       */
      virtual void Init() {}

      /**
       * @brief Lifecycle hook called after each registered window has
       *        been drawn and invalidated, just before the event loop
       *        begins.
       * @note At this point all registered windows are visible on screen
       *       and the app server has processed their creation.
       *
       * Use this to register menus or perform other actions that depend
       * on the windows being fully displayed. The default implementation
       * does nothing.
       */
      virtual void OnReady() {}

      /**
       * @brief Called when a registered window's close button is pressed.
       * @param window The window that received the close event.
       *
       * The default implementation calls @ref RemoveWindow on the closed
       * window, and calls @ref RequestExit when the last window has been
       * removed. Override to add cleanup logic, prevent closing, or
       * customise multi-window shutdown behaviour.
       */
      virtual void OnClose(Window& window);

      /**
       * @brief Called when a registered window is resized.
       * @param window The window that was resized.
       * @param width The new content width in pixels.
       * @param height The new content height in pixels.
       *
       * The default implementation clears the window's surface to its
       * configured content color, then calls @ref Window::DrawAll and
       * @ref Window::Invalidate to redraw all elements at the new size.
       */
      virtual void OnResize(Window& window, UInt16 width, UInt16 height);

      /**
       * @brief Called when a registered window receives a mouse scroll
       *        event.
       * @param window The window that received the scroll event.
       * @param delta Scroll wheel delta (positive = up, negative = down).
       *
       * The default implementation does nothing.
       */
      virtual void OnScroll(Window& window, Int8 delta);

      /**
       * @brief Called when a registered window loses focus.
       * @param window The window that was deactivated.
       *
       * The default implementation calls @ref Window::ClearFocus on the
       * deactivated window.
       */
      virtual void OnDeactivated(Window& window);

      /**
       * @brief Called when a menu action is invoked from a registered
       *        window's context.
       * @param window The window that owned the menu context.
       * @param actionID The action ID that was invoked.
       *
       * Override to handle actions contributed by the @ref App menu
       * providers. The default implementation does nothing.
       */
      virtual void OnMenuAction(Window& window, UInt32 actionID);

      /**
       * @brief Called for events not consumed by element dispatch or the
       *        built-in hooks.
       * @param window The window that received the event.
       * @param event The unhandled @ref WindowEvent.
       *
       * Override for custom event handling without replacing the entire
       * event loop.
       */
      virtual void OnEvent(Window& window, const WindowEvent& event);

    private:
      /**
       * @brief Loads the system UI font from the app server and installs
       *        it as the default font on @ref UI::Canvas.
       * @note Called once per process at the start of the first
       *       @ref App constructor.
       */
      void _loadSystemFont();

      KernelClient _kernel;

      /**
       * @brief Registered windows. Slots are dense; @ref _windowCount
       *        marks the live prefix.
       */
      Window* _windows[MaxWindows] = {};

      /**
       * @brief Number of registered windows in @ref _windows.
       */
      Size _windowCount = 0;

      /**
       * @brief Controls the event loop; set to `false` by
       *        @ref RequestExit.
       */
      bool _running = true;
  };
}
