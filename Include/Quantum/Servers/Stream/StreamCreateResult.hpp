/**
 * @file Include/Quantum/Servers/Stream/StreamCreateResult.hpp
 * @brief Declares @ref StreamCreateResult.
 * @author Brandon Belna (<bbelna@aol.com>)
 * @copyright
 *   Copyright © 2025-2026 The Quantum Software Project. All rights reserved.
 *   Licensed under the GNU General Public License v2.0-only. See LICENSE.md
 *   for details.
 */

#pragma once

#include "StreamTypes.hpp"

namespace Quantum::Servers::Stream {
  /**
   * @brief Result of a @ref StreamOperation::CreateStream request.
   */
  struct StreamCreateResult {
    /**
     * @brief `true` if the stream was created successfully.
     */
    bool Success;

    /**
     * @brief The shared buffer ID backing the new stream. The caller
     *        attaches this buffer to read or write the ring data.
     */
    SharedBufferID BufferID;
  };
}
