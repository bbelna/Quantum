/**
 * @file Include/Quantum/ABI/ABIRequestWithReplyPort.hpp
 * @brief Declares @ref @QABI::ABIRequestWithReplyPort.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/Core/Types.hpp>

#include "ABIRequest.hpp"

namespace Quantum::ABI {
  /**
   * @brief @ref ABIRequest for operations that require a reply port.
   * @tparam OperationType
   *   The type of the `Operation` field, which specifies the operation to
   *   perform. Defaults to @ref UInt32.
   */
  template <typename OperationType = UInt32>
  struct ABIRequestWithReplyPort : public ABIRequest<OperationType> {
    /**
     * @brief @ref IPCPortID for the reply channel.
     * @note The server should send the response to this port.
     */
    IPCPortID ReplyPortID;
  };
}
