/**
 * @file Include/Quantum/Servers/Run/RunServerRequest.hpp
 * @brief Base request type aliases for the run server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include <Quantum/ABI.hpp>

#include "RunServerOperation.hpp"

namespace Quantum::Servers::Run {
  /**
   * @brief Base request type for fire-and-forget run operations.
   */
  using RunServerRequest = ABIRequest<RunServerOperation>;

  /**
   * @brief Base request type for run operations that expect a reply.
   */
  using RunServerRequestWithReply = ABIRequestWithReplyPort<RunServerOperation>;
}
