/**
 * @file Apps/Terminal/Terminal.hpp
 * @brief Declares @ref @QApps::Terminal::Terminal.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <TerminalTypes.hpp>

namespace Quantum::Apps::Terminal {
  /**
   * @brief Implements the @ref Terminal @ref App.
   */
  class Terminal : public App {
    public:
      /**
       * @brief Creates a new @ref Terminal.
       */
      Terminal();

      /**
       * @brief Destroys the @ref Terminal.
       * @note Closes child @ref Stream instances.
       */
      ~Terminal() override;

      /**
       * @brief Custom event loop that polls both window events and child
       *        stdout concurrently.
       * @return Exit code (0 on success, negative on error).
       */
      int Run() override;

    protected:
      /**
       * @brief Initialises the terminal and spawns the child shell
       *        process.
       */
      void Init() override;

      /**
       * @brief Registers the Shell's menus after the window is visible.
       */
      void OnReady() override;

      /**
       * @brief Resizes the terminal to fit the new content area.
       * @param window The window that was resized.
       * @param width The new content width in pixels.
       * @param height The new content height in pixels.
       */
      void OnResize(
        Window& window,
        UInt16 width,
        UInt16 height
      ) override;

      /**
       * @brief Scrolls the terminal viewport.
       * @param window The window that received the scroll event.
       * @param delta Scroll wheel delta.
       */
      void OnScroll(
        Window& window,
        Int8 delta
      ) override;

      /**
       * @brief Handles a menu action invoked from the system menu bar.
       * @param window The window that owned the menu context.
       * @param actionID The action ID.
       */
      void OnMenuAction(
        Window& window,
        UInt32 actionID
      ) override;

    private:
      /**
       * @brief @ref ContextClient instance.
       * @see @ref ContextClient
       */
      ContextClient _context;

      /**
       * @brief @ref FileSystemClient instance.
       * @see @ref FileSystemClient
       */
      FileSystemClient _fileSystem;

      /**
       * @brief @ref StreamClient instance.
       * @see @ref StreamClient
       */
      StreamClient _streams;

      /**
       * @brief @ref RunClient instance.
       * @see @ref RunClient
       */
      RunClient _run;

      /**
       * @brief The @ref App @ref Window.
       * @see @ref Window
       */
      Window _window;

      /**
       * @brief The @ref Console element used for display.
       */
      Console _console;

      /**
       * @brief The @ref Menu provider ID, or `0` if not registered.
       */
      UInt32 _menuProviderID = 0;

      /**
       * @brief Parent's write-end of the child's in stream.
       */
      Stream _inStream;

      /**
       * @brief Parent's read-end of the child's out stream.
       */
      ConsoleStreamHost _outStream;

      /**
       * @brief Whether the child @ref Process has exited.
       */
      bool _childExited = false;

      /**
       * @brief Pointer to the open @ref Dialog, or `nullptr`.
       */
      Dialog* _dialog = nullptr;

      /**
       * @brief The parsed @ref BitmapFont, or empty if loading failed.
       */
      BitmapFont _font = {};

      /**
       * @brief Address of the @ref BitmapFont file data.
       */
      UInt8* _fontData = 0;

      /**
       * @brief Loads the @ref BitmapFont from the filesystem and sets it on the
       *        @ref Terminal @ref Window @ref Canvas.
       */
      void _loadFont();

      /**
       * @brief Spawns the `qsh` child process with inherited streams.
       * @return `true` if the child was spawned successfully.
       */
      bool _spawnChild();

      /**
       * @brief Registers the Shell's focus menu and standard menu
       *        contributions with the context server.
       */
      void _registerMenus();

      /**
       * @brief Unregisters the Shell's menu provider.
       */
      void _unregisterMenus();
  };
}

QUANTUM_APP(Terminal)
