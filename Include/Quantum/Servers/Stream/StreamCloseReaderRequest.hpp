/**
 * @file Include/Quantum/Servers/Stream/StreamCloseReaderRequest.hpp
 * @brief Declares @ref StreamCloseReaderRequest.
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
   * @brief Request to close the reader side of a stream.
   */
  struct StreamCloseReaderRequest : public StreamRequest {
    /**
     * @brief The shared buffer ID of the stream to close.
     */
    SharedBufferID BufferID;
  };
}
