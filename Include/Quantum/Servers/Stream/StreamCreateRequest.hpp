/**
 * @file Include/Quantum/Servers/Stream/StreamCreateRequest.hpp
 * @brief Declares @ref StreamCreateRequest.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "StreamTypes.hpp"
#include "StreamRequest.hpp"

namespace Quantum::Servers::Stream {
  /**
   * @brief Request to create a new stream.
   */
  struct StreamCreateRequest : public StreamRequestWithReply {
    /**
     * @brief Desired buffer size in bytes. A value of `0` uses the default
     *        size (@ref Quantum::Streaming::DefaultStreamBufferSize).
     */
    UInt32 BufferSize;
  };
}
