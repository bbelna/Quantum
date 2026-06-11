/**
 * @file Include/Quantum/Servers/Stream/StreamRequest.hpp
 * @brief Base request type aliases for the stream server.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "StreamTypes.hpp"
#include "StreamOperation.hpp"

namespace Quantum::Servers::Stream {
  /**
   * @brief Base request type for fire-and-forget stream operations.
   */
  using StreamRequest = ABIRequest<StreamOperation>;

  /**
   * @brief Base request type for stream operations that expect a reply.
   */
  using StreamRequestWithReply = ABIRequestWithReplyPort<StreamOperation>;
}
