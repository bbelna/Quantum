/**
 * @file Include/Quantum/Core/Singleton.hpp
 * @brief Declares and implements @ref @QCore::Singleton.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "Types.hpp"

namespace Quantum::Core {
  /**
   * @brief Base class for a singleton.
   */
  template<typename T>
  class Singleton {
    public:
      /**
       * @brief Gets the singleton instance.
       * @return The singleton instance.
       */
      static T& Instance() {
        static T instance;

        return instance;
      }

      /**
       * @brief Deleted copy constructor prevent copying.
       */
      Singleton(const Singleton&) = delete;

      /**
       * @brief Deleted assignment operator to prevent copying.
       */
      Singleton& operator=(const Singleton&) = delete;

    protected:
      /**
       * @brief Protected default constructor to prevent external instantiation.
       */
      Singleton() = default;

      /**
       * @brief Protected default destructor to prevent external destruction.
       */
      ~Singleton() = default;
  };
}
