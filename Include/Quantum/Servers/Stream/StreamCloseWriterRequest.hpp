/**
 * @file Include/Quantum/Servers/Stream/StreamCloseWriterRequest.hpp
 * @brief Declares @ref StreamCloseWriterRequest.
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
   * @brief Request to close the writer side of a stream.
   */
  struct StreamCloseWriterRequest : public StreamRequest {
    /**
     * @brief The shared buffer ID of the stream to close.
     */
    SharedBufferID BufferID;
  };
}
