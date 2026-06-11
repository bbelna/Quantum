/**
 * @file Apps/About.hpp
 * @brief Declares @ref @QApps::About::About.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright (c) 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <AboutTypes.hpp>

namespace Quantum::Apps::About {
  /**
   * @brief Implements the @ref About @ref App.
   */
  class About : public App {
    public:
      /**
       * @brief Creates a new @ref About @ref App.
       */
      About();

    protected:
      /**
       * @brief Sets up the @ref About @ref Window.
       * @see @ref Window, @ref _window
       */
      void Init() override;

    private:
      /**
       * @brief Content padding for the @ref About @ref Window.
       */
      static constexpr UInt16 _padding = 12;

      /**
       * @brief The @ref About @ref App @ref Window.
       */
      Window _window;

      /**
       * @brief The header @ref Label.
       */
      Label _headerLabel;

      /**
       * @brief The first line of copyright @ref Label.
       */
      Label _copyrightLine1;

      /**
       * @brief The second line of copyright @ref Label.
       */
      Label _copyrightLine2;
  };
}
