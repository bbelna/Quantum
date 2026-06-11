/**
 * @file Bootloader/Platform/PC/VGASpinner.hpp
 * @brief Declares @ref @QBtldr::Platform::PC::VGASpinner.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Platform/PC/PCTypes.hpp>

namespace Quantum::Bootloader::Platform::PC {
  /**
   * @brief Spinner implementation for PC VGA displays.
   */
  class VGASpinner : public ISpinner {
    public:
      /**
      * @brief Shows the spinner.
      */
      void Show() override;

      /**
      * @brief Advances the spinner animation if enough time has elapsed.
      *
      * Should be called periodically while the spinner is shown to update the
      * animation.
      */
      void Tick() override;

      /**
      * @brief Hides the spinner and resets its state.
      */
      void Hide() override;

    private:
      /**
       * @brief The animation frames for the spinner.
       */
      constexpr static char _frames[4] = {'|', '/', '-', '\\'};

      /**
       * @brief The VGA driver used to draw the spinner.
       */
      BIOSVGADriver* _vgaDriver = nullptr;

      /**
       * @brief Indicates whether the spinner is currently visible.
       */
      bool _visible = false;

      /**
       * @brief The current frame index of the spinner animation.
       */
      UInt8 _frameIndex = 0;

      /**
       * @brief The timestamp of the last tick.
       */
      UInt32 _lastTick = 0;

      /**
       * @brief Draws a single character at the current cursor position.
       * @param c The character to draw.
       */
      void _put(char c);
  };
}
