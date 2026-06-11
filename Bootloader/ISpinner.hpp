/**
 * @file Bootloader/ISpinner.hpp
 * @brief Declares @ref @QBtldr::ISpinner.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <BootloaderTypes.hpp>

namespace Quantum::Bootloader {
  /**
   * @brief Interface for a spinner used in the bootloader.
   */
  class ISpinner {
    public:
      /**
      * @brief Destroys this @ref ISpinner instance.
      */
      virtual ~ISpinner() = default;

      /**
      * @brief Shows the spinner.
      */
      virtual void Show() = 0;

      /**
      * @brief Advances the spinner animation if enough time has elapsed.
      *
      * Should be called periodically while the spinner is shown to update the
      * animation.
      */
      virtual void Tick() = 0;

      /**
      * @brief Hides the spinner and resets its state.
      */
      virtual void Hide() = 0;
  };
}
