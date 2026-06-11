/**
 * @file Include/Quantum/App/Dialog.hpp
 * @brief Declares @ref @QApp::Dialog.
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
#include <Quantum/Components/Button.hpp>
#include <Quantum/Components/Label.hpp>

#include "DialogButton.hpp"
#include "Window.hpp"

namespace Quantum::App {
  /**
   * @brief A modal dialog @ref Window.
   *
   * The dialog creates its own @ref Window, establishes a modal relationship
   * with the parent @ref Window (blocking interaction until dismissed), and
   * runs its own event loop.
   *
   * The caller's thread blocks in @ref Dialog::Show until @ref Dialog::Close
   * is called (typically from a button callback).
   */
  class Dialog {
    public:
      static constexpr UInt16 Padding = 16;
      static constexpr UInt16 ButtonWidth = 80;
      static constexpr UInt16 ButtonHeight = 28;
      static constexpr UInt16 ButtonSpacing = 8;
      static constexpr UInt16 GlyphWidth = 8;
      static constexpr UInt16 GlyphHeight = 14;
      static constexpr UInt32 BackgroundColor
        = ::Quantum::Theme::WindowBackground;
      static constexpr UInt32 TextColor
        = ::Quantum::Theme::TextForeground;
      static constexpr Size MaxButtons = 8;

      /**
       * @brief Spacing between the header and the message body.
       */
      static constexpr UInt16 HeaderSpacing = 8;

      /**
       * @brief Creates a @ref Dialog owned by a parent @ref Window.
       * @param parent The parent @ref Window to block while the @ref Dialog is
       *               open.
       * @param title The @ref Dialog @ref Window title.
       * @param header The header text.
       * @param message The message text displayed in the @ref Dialog body.
       *                Supports `\n` for line breaks.
       * @param buttons List of @ref DialogButton instances to use.
       * @param buttonCount Number of entries in @p buttons.
       */
      Dialog(
        Window& parent,
        const char* title,
        const char* header,
        const char* message,
        const DialogButton* buttons,
        Size buttonCount
      );

      /**
       * @brief Constructs a standalone @ref Dialog with no parent @ref Window.
       * @param title The @ref Dialog @ref Window title.
       * @param header The header text.
       * @param message The message text displayed in the @ref Dialog body.
       *                Supports `\n` for line breaks.
       * @param buttons List of @ref DialogButton instances to use.
       * @param buttonCount Number of entries in @p buttons.
       */
      Dialog(
        const char* title,
        const char* header,
        const char* message,
        const DialogButton* buttons,
        Size buttonCount
      );

      /**
       * @brief Destroys the dialog and releases modal state.
       */
      ~Dialog();

      /**
       * @brief Shows the dialog and runs its modal event loop. Blocks
       *        until `Close()` is called.
       */
      void Show();

      /**
       * @brief Creates and displays the dialog window without entering a
       *        blocking event loop. The caller must pump events by
       *        calling @ref TryPumpEvent in its own loop and check
       *        @ref IsOpen to detect dismissal.
       *
       * Call @ref Dismiss to clean up when finished.
       */
      void ShowNonBlocking();

      /**
       * @brief Polls for a single dialog event and handles it
       *        (non-blocking). Call this repeatedly from the caller's
       *        event loop while @ref IsOpen returns `true`.
       * @return `true` if an event was processed.
       */
      bool TryPumpEvent();

      /**
       * @brief Returns whether the dialog is currently open.
       */
      bool IsOpen() const { return _open; }

      /**
       * @brief Cleans up a non-blocking dialog's window and modal state.
       *        Call after @ref IsOpen returns `false`.
       */
      void Dismiss();

      /**
       * @brief Closes the dialog, ending the modal event loop. Typically
       *        called from a button callback.
       */
      void Close();

    private:
      Window* _parent;

      char _title[64];

      char _header[64];

      char _message[256];

      DialogButton _buttons[MaxButtons];

      Size _buttonCount;

      bool _open = false;

      /**
       * @brief The dialog window (owned, created by ShowNonBlocking).
       */
      Window* _dialogWindow = nullptr;

      /**
       * @brief Button UI elements for the non-blocking path.
       */
      Components::Button* _buttonElements[MaxButtons] = {};

      /**
       * @brief Loads a system font by index from the app server.
       * @param index Font index.
       * @param font Output font structure.
       * @return `true` if the font was loaded successfully.
       */
      bool _loadSystemFont(Size index, Fonts::BitmapFont& font);
  };
}
