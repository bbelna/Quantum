/**
 * @file Include/Quantum/ABI/ABIRequest.hpp
 * @brief Declares @ref @QABI::ABIRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "ABITypes.hpp"

namespace Quantum::ABI {
  /**
   * @brief Base ABI request structure.
   * @tparam OperationType
   *   The type of the `Operation` field, which specifies the operation to
   *   perform. Defaults to @ref UInt32.
   */
  template <typename OperationType = UInt32>
  struct ABIRequest {
    /**
     * @brief ABI version of the request.
     */
    UInt32 ABIVersion;

    /**
     * @brief The operation to perform.
     */
    OperationType Operation;
  };
}
