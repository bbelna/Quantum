/**
 * @file Include/Quantum/Core/IInitializer.hpp
 * @brief Declares @ref @QCore::IInitializer.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Core {
  /**
   * @brief Abstract initializer interface.
   * @tparam OptionsType The type carrying initialization options.
   * @tparam ReturnType The type returned by the initializer. Defaults to
   *                    `void`.
   */
  template<typename OptionsType, typename ReturnType = void>
  class IInitializer {
    public:
      /**
       * @brief Initializes the component with the provided options.
       * @param options The initialization options.
       * @return The result of the initialization.
       */
      virtual ReturnType Initialize(OptionsType options) = 0;
  };
}
