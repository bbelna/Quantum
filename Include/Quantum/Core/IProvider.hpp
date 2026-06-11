/**
 * @file Include/Quantum/Core/IProvider.hpp
 * @brief Declares @ref @QCore::IProvider.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

namespace Quantum::Core {
  /**
   * @brief Abstract provider interface.
   * @tparam ReturnType The type returned by the provider.
   * @tparam OptionsType The type carrying provision options. Defaults to
   *                     `void` if no options are needed.
   */
  template<typename ReturnType, typename OptionsType = void>
  class IProvider {
    public:
      /**
       * @brief Provides a `ReturnType` using the given options.
       * @param options The options to use when providing the `ReturnType`.
       * @return The `ReturnType` provided using the given options.
       */
      virtual ReturnType Provide(OptionsType options) = 0;
  };

  /**
   * @brief Specialization of @ref IProvider for providers that take no
   *        options.
   * @tparam ReturnType The type returned by the provider.
   */
  template<typename ReturnType>
  class IProvider<ReturnType, void> {
    public:
      /**
       * @brief Provides a `ReturnType`.
       * @return The `ReturnType` provided by this provider.
       */
      virtual ReturnType Provide() = 0;
  };
}
