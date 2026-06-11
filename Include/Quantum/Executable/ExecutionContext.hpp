/**
 * @file Include/Quantum/Executable/ExecutionContext.hpp
 * @brief Declares @ref @QX::ExecutionContext.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

namespace Quantum::Executable {
  /**
   * @brief @ref Executable execution context structure.
   */
  struct ExecutionContext {
    /**
     * @brief Arguments passed to the @ref Executable.
     */
    const char** Arguments;

    /**
     * @brief Number of arguments in @ref Arguments.
     */
    Size ArgumentCount;

    /**
     * @brief Environment variables passed to the @ref Executable.
     */
    const char** Environment;
  };
}
