/**
 * @file Apps/UIShowcase/UIShowcase.hpp
 * @brief Declares @ref @QDemos::UIShowcase::UIShowcase.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <UIShowcaseTypes.hpp>

namespace Quantum::Demos::UIShowcase {
  /**
   * @brief The UI showcase demo app.
   */
  class UIShowcase : public App {
    public:
      /**
       * @brief Creates a new @ref UIShowcase.
       */
      UIShowcase();

      /**
       * @brief Destroys the @ref UIShowcase.
       */
      ~UIShowcase() override;

    protected:
      /**
       * @brief Adds UI elements to the window and registers callbacks.
       */
      void Init() override;

    private:
      /**
       * @brief The app's primary window. Declared first so its surface
       *        is available to the element members below.
       */
      Window _window;

      Label _heading;

      Label _inputLabel;

      TextInput _textInput;

      Button _okButton;

      Button _cancelButton;

      /**
       * @brief Buffer holding the raw PSF file data. Owned by this object.
       */
      UIntPtr _fontBuffer = 0;

      /**
       * @brief The parsed font loaded from disk.
       */
      BitmapFont _loadedFont = {};

      /**
       * @brief Loads the font from the floppy and sets it on the surface.
       */
      void _loadFont();
  };
}
